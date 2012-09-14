/*
 * Author: Nico Golde <nico@ngolde.de>
 * Fr Mai 25 13:12:35 CEST 2007 
 * small test program for libacpi (http://www.ngolde.de/libacpi.html)
 * the include needs to be changed to link against the share lib, just didn't do
 * it because I don't want to install it every time before testing :)
 */

#include "libacpi.h"
#include <stdio.h>
#include <stdlib.h>

int
main(void){
	int i=0;
	int acstate, battstate, thermstate, fanstate;

	/* the global structure is _the_ acpi structure here */
	global_t *global = malloc (sizeof (global_t));
	battery_t *binfo;
	adapter_t *ac = &global->adapt;
	thermal_t *tp;
	fan_t *fa;

	if(check_acpi_support() == NOT_SUPPORTED){
		printf("No acpi support for your system?\n");
		return -1;
	}

	/* initialize battery, thermal zones, fans and ac state */
	battstate = init_acpi_batt(global);
	thermstate = init_acpi_thermal(global);
	fanstate = init_acpi_fan(global);
	acstate = init_acpi_acadapt(global);

	if(acstate == SUCCESS && ac->ac_state == P_BATT)
		printf("AC adapter: off-line\n");
	else if(acstate == SUCCESS && ac->ac_state == P_AC)
		printf("AC adapter: on-line\n");
	else printf("AC information:\t\tnot supported\n");

	if(battstate == SUCCESS){
		for(i=0;i<global->batt_count;i++){
			binfo = &batteries[i];
			/* read current battery values */
			read_acpi_batt(i);

			if(binfo->present)
				printf("\n%s:\tpresent: %d\n"
						"\tdesign capacity: %d\n"
						"\tlast full capacity: %d\n"
						"\tdesign voltage: %d\n"
						"\tpresent rate: %d\n"
						"\tremaining capacity: %d\n"
						"\tpresent voltage: %d\n"
						"\tcharge state: %d\n"
						"\tbattery state: %d\n"
						"\tpercentage: %d%%\n"
						"\tremaining charge time: %02d:%02d h\n"
						"\tremaining time: %02d:%02d h\n",
						binfo->name, binfo->present, binfo->design_cap,
						binfo->last_full_cap, binfo->design_voltage,
						binfo->present_rate, binfo->remaining_cap,
						binfo->present_voltage, binfo->charge_state,
						binfo->batt_state, binfo->percentage, 
						binfo->charge_time / 60, binfo->charge_time % 60,
						binfo->remaining_time / 60, binfo->remaining_time % 60);
		}
	} else printf("Battery information:\tnot supported\n");
	
	if(thermstate == SUCCESS){
		for(i=0; i<global->thermal_count; i++){
			/* read current thermal zone values */
			read_acpi_zone(i, global);
			tp = &thermals[i];
			if(tp->frequency == DISABLED)
				printf("\n%s:\ttemperature: %d C\n"
					"\tfrequency: disabled\n"
					"\tmode: %d\n"
					"\tstate: %d\n",
					tp->name, tp->temperature, tp->therm_mode, tp->therm_state);

			else printf("\n%s:\ttemperature: %d °C\n"
					"\tfrequency: %d seconds\n"
					"\tmode: %d\n"
					"\tstate: %d\n",
					tp->name, tp->temperature,tp->frequency, tp->therm_mode, tp->therm_state);
		}
		if(global->thermal_count == 1)
			printf("Temperature: %d °C\n", global->temperature);
	} else printf("Thermal information not supported\n");

	if(fanstate == SUCCESS){
		for(i=0; i<global->fan_count; i++){
			/* read fan state */
			read_acpi_fan(i);
			fa = &fans[i];
			printf("\n%s:\tstate: %d\n", fa->name, fa->fan_state);
		}
	} else printf("Fan information:\tnot supported\n");

	free(global);

	return 0;
}
