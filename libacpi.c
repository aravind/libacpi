/*
 * (C)opyright 2007 Nico Golde <nico@ngolde.de>
 * See LICENSE file for license details
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <stddef.h>

#include "libacpi.h"
#include "list.h"

static int read_acpi_battinfo(const int num);
static int read_acpi_battalarm(const int num);
static int read_acpi_battstate(const int num);
static void read_acpi_thermalzones(global_t *globals);

typedef struct {
	char * value;
	size_t offset;
} acpi_value_t;

static acpi_value_t
battinfo_values[] = {
	{ "last full capacity:", offsetof(battery_t, last_full_cap) },
	{ "design voltage:", offsetof(battery_t, design_voltage) },
	{ "design capacity warning:", offsetof(battery_t, design_warn) },
	{ "design capacity low:", offsetof(battery_t, design_low) },
	{ "capacity granularity 1:", offsetof(battery_t, design_level1) },
	{ "capacity granularity 2:", offsetof(battery_t, design_level2) },
	{ NULL, 0 }
};

static acpi_value_t
battstate_values[] = {
	{ "present rate:", offsetof(battery_t, present_rate) },
	{ "remaining capacity:", offsetof(battery_t, remaining_cap) },
	{ "present voltage:", offsetof(battery_t, present_voltage) },
	{ NULL, 0 }
};

/* given a buffer for example from a file, search for key
 * and return a pointer to the value of it. On error return NULL*/
static char *
scan_acpi_value(const char *buf, const char *key){
	char *ptr = NULL;
	char *tmpbuf = NULL;
	char *tmpkey = NULL;
	char *tmpval = NULL;

	if((tmpbuf = strdup(buf)) == NULL)
		return NULL;

	/* jump to the key in buffer */
	if((tmpkey = strstr(tmpbuf, key))) {
		/* jump behind the key, whitespaces and tabs */
		for(tmpkey += strlen(key); *tmpkey && (*tmpkey == ' ' || *tmpkey == '\t'); tmpkey++);
		for(tmpval = tmpkey; *tmpval && *tmpval != ' ' &&
				*tmpval != '\t' && *tmpval != '\n' &&
				*tmpval != '\r'; tmpval++);
		if(tmpval)
			*tmpval = '\0';

		if((ptr = strdup(tmpkey)) == NULL) {
			free(tmpbuf);
			return NULL;
		}
	}
	free(tmpbuf);
	return ptr;
}

/* reads a file into a buffer and returns a pointer to it, or NULL on error */
static char *
get_acpi_content(const char *file){
	FILE *input = NULL;
	char *buf = NULL;
	int read = 0;

	if((buf = malloc(MAX_BUF + 1)) == NULL)
		return NULL;
	if((input = fopen(file, "r")) == NULL)
		return NULL;

	read = fread(buf, 1, MAX_BUF, input);
	if(read > 0) buf[read - 1] = '\0';
	else buf[0] = '\0'; /* I would consider it a kernel bug if that happens */

	fclose(input);
	return buf;
}

/* returns the acpi version or NOT_SUPPORTED(negative value) on failure */
static int
get_acpi_version(void){
	long ret = -1;
	char *tmp = get_acpi_content(PROC_ACPI "info");
	char *version = NULL;
	
	if(!tmp) {
		tmp = get_acpi_content("/sys/module/acpi/parameters/acpica_version");
		if (tmp) {
			long ret = strtol(tmp, NULL, 10);
			free(tmp);
			return ret;
		} else {
			return NOT_SUPPORTED;
		}
	}
	if((version = scan_acpi_value(tmp, "version:")) == NULL){
		free(tmp);
		return NOT_SUPPORTED;
	}
	ret = strtol(version, NULL, 10);
	free(tmp);
	free(version);
	return ret;
}

/* check if acpi is supported on the system, return 0 on success
 * and -1 if not */
int
check_acpi_support(void){
	int version = get_acpi_version();

	/* we don't support 2.4 kernel versions TODO */
	if(version == NOT_SUPPORTED || version < 20020214)
		return NOT_SUPPORTED;
	return SUCCESS;
}

/* reads existent battery directories and starts to fill the battery
 * structure. Returns 0 on success, negative values on error */
int
init_acpi_batt(global_t *globals){
	char *names[MAX_ITEMS];
	battery_t *binfo;
	list_t *lst = NULL;
	node_t *node = NULL;
	int i = 0;

	globals->batt_count = 0;
	if((lst = dir_list(PROC_ACPI "battery")) == NULL || !lst->top)
		return NOT_SUPPORTED;
	for(node = lst->top; node; node=node->next){
		if((names[globals->batt_count] = strdup(node->name)) == NULL){
			delete_list(lst);
			return ALLOC_ERR;
		}
		globals->batt_count++;
	}

	if(globals->batt_count > MAX_ITEMS) return ITEM_EXCEED;

	/* A quick insertion sort, to sort battery names */
	{
		char *tmp1, *tmp2;
		int x,y;
		for (x = 1; x < globals->batt_count; x++) {
			tmp1 = names[x];
			y = x - 1;
			while ((y >= 0) && ((strcmp (tmp1, names[y])) < 0)) {
				tmp2 = names[y + 1];
				names[y + 1] = names[y];
				names[y] = tmp2;
			}
		}
	}

	for (i=0; i < globals->batt_count && i < MAX_ITEMS; i++){
		binfo = &batteries[i];
		snprintf(binfo->name, MAX_NAME, "%s", names[i]);
		snprintf(binfo->state_file, MAX_NAME, PROC_ACPI "battery/%s/state", names[i]);
		snprintf(binfo->info_file, MAX_NAME, PROC_ACPI "battery/%s/info", names[i]);
		snprintf(binfo->alarm_file, MAX_NAME, PROC_ACPI "battery/%s/alarm", names[i]);
		read_acpi_battinfo(i);
		read_acpi_battalarm(i);
		free(names[i]);
	}
	delete_list(lst);
	return SUCCESS;
}

/* reads the acpi state and writes it into the globals structure, void */
void
read_acpi_acstate(global_t *globals){
	adapter_t *ac = &globals->adapt;
	char *buf = NULL;
	char *tmp = NULL;

	if(ac->state_file && (buf = get_acpi_content(ac->state_file)) == NULL){
		ac->ac_state = P_ERR;
		return;
	}
	if((tmp = scan_acpi_value(buf, "state:")) && !strncmp(tmp, "on-line", 7))
		ac->ac_state = P_AC;
	else if(tmp && !strncmp(tmp, "off-line", 8))
		ac->ac_state = P_BATT;
	else ac->ac_state = P_ERR;
	free(buf);
	free(tmp);
}

/* reads the name of the ac-adapter directory and fills the adapter_t
 * structure with the name and the state-file. Return 0 on success, negative values on errors */
int
init_acpi_acadapt(global_t *globals){
	list_t *lst = NULL;
	adapter_t *ac = &globals->adapt;

	if((lst = dir_list(PROC_ACPI "ac_adapter")) == NULL || !lst->top)
		return NOT_SUPPORTED;

	if((!lst->top->name || ((ac->name = strdup(lst->top->name)) == NULL))){
		delete_list(lst);
		return ALLOC_ERR;
	}
	snprintf(ac->state_file, MAX_NAME, PROC_ACPI "ac_adapter/%s/state", ac->name);
	delete_list(lst);
	read_acpi_acstate(globals);
	return SUCCESS;
}

/* read acpi information for fan num, returns 0 on success and negative values on errors */
int
read_acpi_fan(const int num){
	char *buf = NULL;
	char *tmp = NULL;
	fan_t *info = &fans[num];

	if(num > MAX_ITEMS) return ITEM_EXCEED;

	/* scan state file */
	if((buf = get_acpi_content(info->state_file)) == NULL)
		info->fan_state = F_ERR;

	if(!buf || (tmp = scan_acpi_value(buf, "status:")) == NULL){
		info->fan_state = F_ERR;
		return NOT_SUPPORTED;
	}
	if (tmp[0] == 'o' && tmp[1] == 'n') info->fan_state = F_ON;
	else if(tmp[0] == 'o' && tmp[1] == 'f') info->fan_state = F_OFF;
	else info->fan_state = F_ERR;
	free(buf);
	free(tmp);
	return SUCCESS;
}

/* read all fans, fill the fan structures */
static void
read_acpi_fans(global_t *globals){
	int i;
	for(i = 0; i < globals->fan_count; i++)
		read_acpi_fan(i);
}

/* reads the names of the fan directories, fills fan_t,
 * return 0 on success, negative values on errors */
int
init_acpi_fan(global_t *globals){
	char *names[MAX_ITEMS];
	list_t *lst = NULL;
	node_t *node = NULL;
	int i = 0;
	fan_t *finfo = NULL;
	globals->fan_count = 0;

	if((lst = dir_list(PROC_ACPI "fan")) == NULL || !lst->top)
		return NOT_SUPPORTED;
	for(node = lst->top; node; node = node->next){
		if((names[globals->fan_count] = strdup(node->name)) == NULL){
			delete_list(lst);
			return ALLOC_ERR;
		}
		globals->fan_count++;
	}

	if(globals->fan_count > MAX_ITEMS) return ITEM_EXCEED;

	for (; i < globals->fan_count && i < MAX_ITEMS; i++){
		finfo = &fans[i];
		snprintf(finfo->name, MAX_NAME, "%s", names[i]);
		snprintf(finfo->state_file, MAX_NAME, PROC_ACPI "fan/%s/state", names[i]);
		free(names[i]);
	}
	delete_list(lst);
	read_acpi_fans(globals);
	return SUCCESS;
}

/* reads the name of the thermal-zone directory and fills the adapter_t
 * structure with the name and the state-file. Return 0 on success, negative values on errors */
int
init_acpi_thermal(global_t *globals){
	char *names[MAX_ITEMS];
	list_t *lst = NULL;
	node_t *node = NULL;
	thermal_t *tinfo = NULL;
	int i = 0;
	globals->thermal_count = 0;

	if((lst = dir_list(PROC_ACPI "thermal_zone")) == NULL)
		return NOT_SUPPORTED;
	for(node = lst->top; node; node = node->next){
		if((names[globals->thermal_count] = strdup(node->name)) == NULL){
			delete_list(lst);
			return ALLOC_ERR;
		}
		globals->thermal_count++;
	}

	if(globals->thermal_count > MAX_ITEMS) return ITEM_EXCEED;

	for (; i < globals->thermal_count && i < MAX_ITEMS; i++){
		tinfo = &thermals[i];
		snprintf(tinfo->name, MAX_NAME, "%s", names[i]);
		snprintf(tinfo->state_file, MAX_NAME, PROC_ACPI "thermal_zone/%s/state", names[i]);
		snprintf(tinfo->temp_file, MAX_NAME, PROC_ACPI "thermal_zone/%s/temperature", names[i]);
		snprintf(tinfo->cooling_file, MAX_NAME, PROC_ACPI "thermal_zone/%s/cooling_mode", names[i]);
		snprintf(tinfo->freq_file, MAX_NAME, PROC_ACPI "thermal_zone/%s/polling_frequency", names[i]);
		snprintf(tinfo->trips_file, MAX_NAME, PROC_ACPI "thermal_zone/%s/trip_points", names[i]);
		free(names[i]);
	}
	delete_list(lst);
	read_acpi_thermalzones(globals);
	return SUCCESS;
}

/* checks the string state and sets the thermal state, returns void */
static void
thermal_state(const char *state, thermal_t *info){
	if(state[0] == 'o')
		info->therm_state = T_OK;
	else if(!strncmp (state, "crit", 4))
		info->therm_state = T_CRIT;
	else if (!strncmp (state, "hot", 3))
		info->therm_state = T_HOT; else if (!strncmp (state, "pas", 3))
		info->therm_state = T_PASS;
	else
		info->therm_state = T_ACT;
}

/* checks the string tmp and sets the cooling mode */
static void
fill_cooling_mode(const char *tmp, thermal_t *info){
	if(tmp[0] == 'a')
		info->therm_mode = CO_ACT;
	else if(tmp[0]  == 'p')
		info->therm_mode = CO_PASS;
	else info->therm_mode = CO_CRIT;
}

/* reads values for thermal_zone num, return 0 on success, negative values on error */
int
read_acpi_zone(const int num, global_t *globals){
	char *buf = NULL;
	char *tmp = NULL;
	thermal_t *info = &thermals[num];

	if(num > MAX_ITEMS) return ITEM_EXCEED;

	/* scan state file */
	if((buf = get_acpi_content(info->state_file)) == NULL)
		info->therm_state = T_ERR;

	if(buf && (tmp = scan_acpi_value(buf, "state:")))
			thermal_state(tmp, info);
	free(tmp);
	free(buf);

	/* scan temperature file */
	if((buf = get_acpi_content(info->temp_file)) == NULL)
		info->temperature = NOT_SUPPORTED;

	if(buf && (tmp = scan_acpi_value(buf, "temperature:"))){
		info->temperature = strtol(tmp, NULL, 10);
		/* if we just have one big thermal zone, this will be the global temperature */
		if(globals->thermal_count == 1)
			globals->temperature = info->temperature;
	}
	free(tmp);
	free(buf);

	/* scan cooling mode file */
	if((buf = get_acpi_content(info->cooling_file)) == NULL)
		info->therm_mode = CO_ERR;
	if(buf && (tmp = scan_acpi_value(buf, "cooling mode:")))
		fill_cooling_mode(tmp, info);
	else info->therm_mode = CO_ERR;
	free(tmp);
	free(buf);

	/* scan polling_frequencies file */
	if((buf = get_acpi_content(info->freq_file)) == NULL)
		info->frequency = DISABLED;
	if(buf && (tmp = scan_acpi_value(buf, "polling frequency:")))
		info->frequency = strtol(tmp, NULL, 10);
	else info->frequency = DISABLED;
	free(tmp);
	free(buf);

	/* TODO: IMPLEMENT TRIP POINTS FILE */

	return SUCCESS;
}

/* read all thermal zones, fill the thermal structures */
static void
read_acpi_thermalzones(global_t *globals){
	int i;
	for(i = 0; i < globals->thermal_count; i++)
		read_acpi_zone(i, globals);
}

/* fill battery_state for given battery, return 0 on success or negative values on error */
static void
batt_charge_state(battery_t *info){
	int high = info->last_full_cap / 2;
	int med = high / 2;

	if(info->remaining_cap > high)
		info->batt_state = B_HIGH;
	else if(info->remaining_cap <= high && info->remaining_cap > med)
		info->batt_state = B_MED;
	else if(info->remaining_cap <= med && info->remaining_cap > info->design_warn)
		info->batt_state = B_LOW;
	else if(info->remaining_cap <= info->design_warn && info->remaining_cap > info->design_low)
		info->batt_state = B_CRIT;
	else info->batt_state = B_HARD_CRIT;
}

/* fill charge_state of a given battery num, return 0 on success or negative values on error */
static void
fill_charge_state(const char *state, battery_t *info){
	if(state[0] == 'u')
		info->charge_state = C_ERR;
	else if(!strncmp (state, "disch", 5))
		info->charge_state = C_DISCHARGE;
	else if (!strncmp (state, "charge", 6))
		info->charge_state = C_CHARGED;
	else if (!strncmp (state, "chargi", 6))
		info->charge_state = C_CHARGE;
	else
		info->charge_state = C_NOINFO;
}

/* read alarm capacity, return 0 on success, negative values on error */
static int
read_acpi_battalarm(const int num){
	char *buf = NULL;
	char *tmp = NULL;
	battery_t *info = &batteries[num];

	if((buf = get_acpi_content(info->alarm_file)) == NULL)
		return NOT_SUPPORTED;

	if((tmp = scan_acpi_value(buf, "alarm:")) && tmp[0] != 'u')
		info->alarm = strtol(tmp, NULL, 10);
	else
		info->alarm = NOT_SUPPORTED;
	free(buf);
	free(tmp);
	return SUCCESS;
}

/* reads static values for a battery (info file), returns SUCCESS */
static int
read_acpi_battinfo(const int num){
	char *buf = NULL;
	char *tmp = NULL;
	battery_t *info = &batteries[num];
	int i = 0;

	if((buf = get_acpi_content(info->info_file)) == NULL)
		return NOT_SUPPORTED;

	/* you have to read the present value always since a battery can be taken away while
	 * refreshing the data */
	if((tmp = scan_acpi_value(buf, "present:")) && !strncmp(tmp, "yes", 3)) {
		free(tmp);
		info->present = 1;
	} else {
		info->present = 0;
		free(buf);
		return NOT_PRESENT;
	}

	if((tmp = scan_acpi_value(buf, "design capacity:")) && tmp[0] != 'u'){
		info->design_cap = strtol(tmp, NULL, 10);
		/* workaround ACPI's broken way of reporting no battery */
		if(info->design_cap == 655350) info->design_cap = NOT_SUPPORTED;
		free(tmp);
	}
	else info->design_cap = NOT_SUPPORTED;

	for (;battinfo_values[i].value; i++) {
		if ((tmp = scan_acpi_value(buf, battinfo_values[i].value)) && tmp[0] != 'u') {
			*((int *)(((char *)info) + battinfo_values[i].offset)) = strtol(tmp, NULL, 10);
			free(tmp);
		} else {
			*((int *)(((char *)info) + battinfo_values[i].offset)) = NOT_SUPPORTED;
		}
	}

	/* TODO remove debug */
	/* printf("%s\n", buf); */
	free(buf);

	return SUCCESS;
}

/* read information for battery num, return 0 on success or negative values on error */
static int
read_acpi_battstate(const int num){
	char *buf = NULL;
	char *tmp = NULL;
	battery_t *info = &batteries[num];
	unsigned int i = 0;

	if((buf = get_acpi_content(info->state_file)) == NULL)
		return NOT_SUPPORTED;
	
	if((tmp = scan_acpi_value(buf, "present:")) && !strncmp(tmp, "yes", 3)) {
		info->present = 1;
		free(tmp);
	} else {
		info->present = 0;
		free(buf);
		return NOT_PRESENT;
	}

	/* TODO REMOVE DEBUG */
	/* printf("%s\n\n", buf); */

	if((tmp = scan_acpi_value(buf, "charging state:")) && tmp[0] != 'u') {
		fill_charge_state(tmp, info);
		free(tmp);
	} else {
		info->charge_state = C_NOINFO;
	}

	for (;battstate_values[i].value; i++) {
		if ((tmp = scan_acpi_value(buf, battstate_values[i].value)) && tmp[0] != 'u') {
			*((int *)(((char *)info) + battstate_values[i].offset)) = strtol(tmp, NULL, 10);
			free(tmp);
		} else {
			*((int *)(((char *)info) + battstate_values[i].offset)) = NOT_SUPPORTED;
		}
	}

	/* get information from the info file */
	batt_charge_state(info);
	
	free(buf);
	return SUCCESS;
}

/* calculate percentage of battery capacity num */
static void
calc_remain_perc(const int num){
	float lfcap;
	battery_t *info = &batteries[num];
	int perc;

	if(info->remaining_cap < 0){
		info->percentage = NOT_SUPPORTED;
		return;
	}
	else{
		lfcap = info->last_full_cap;
		if(lfcap <= 0) lfcap = 1;
		perc = (int) ((info->remaining_cap / lfcap) * 100.0);
	}
	info->percentage = perc > 100 ? 100 : perc;
}

/* calculate remaining charge time for battery num */
static void
calc_remain_chargetime(const int num){
	battery_t *info = &batteries[num];

	if(info->present_rate < 0 || info->charge_state != C_CHARGE){
		info->charge_time = 0;
		return;
	}
	info->charge_time = (int) ((((float)info->last_full_cap - (float)info->remaining_cap) / info->present_rate) * 60.0);
}

/* calculate remaining time for battery num */
static void
calc_remain_time(const int num){
	battery_t *info = &batteries[num];

	if(info->present_rate < 0 || info->charge_state != C_DISCHARGE){
		info->remaining_time = 0;
		return;
	}
	info->remaining_time = (int) (((float)info->remaining_cap / (float)info->present_rate) * 60.0);
}

/* read/refresh information about a given battery num
 * returns 0 on SUCCESS, negative values on errors */
int
read_acpi_batt(const int num){
	if(num > MAX_ITEMS) return ITEM_EXCEED;
	read_acpi_battstate(num);
	read_acpi_battalarm(num);
	calc_remain_perc(num);
	calc_remain_chargetime(num);
	calc_remain_time(num);
	return SUCCESS;
}
