/*
 * (C)opyright 2007 Nico Golde <nico@ngolde.de>
 * See LICENSE file for license details
 */

/**
 * \file libacpi.h
 * \brief libacpi structures
 */

#ifndef __LIBACPI_H__
#define __LIBACPI_H__

#define PROC_ACPI "/proc/acpi/"
#define LINE_MAX 256
#define MAX_NAME 512
#define MAX_BUF 1024
#define MAX_ITEMS 10

/**
 * \enum return values
 * \brief return values of internal functions
 */
enum {
	ITEM_EXCEED = -5,    /**< maximum item count reached */
	DISABLED = -4,       /**< feature is disabled */
	NOT_PRESENT = -3,    /**< something is not present */
	ALLOC_ERR = -2,      /**< an error occurred while allocating space */
	NOT_SUPPORTED = -1,  /**< a feature is not supported */
	SUCCESS              /**< everything was fine */
};

/**
 * \struct power_state_t
 * \brief power states
 */
typedef enum {
	P_AC,                /**< if computer runs on AC */
	P_BATT,              /**< if computer runs on battery */
	P_ERR                /**< no information can be found */
} power_state_t;

/**
 * \struct thermal_state_t
 * \brief thermal zone states
 */
typedef enum {
	T_CRIT,              /**< zone reports critical temperature, will cause system to go to S4 */
	T_HOT,               /**< zone reports high temperature, will cause system to shutdown immediately */
	T_PASS,              /**< zone is on passive cooling */
	T_ACT,               /**< zone is on active cooling, more power consumption */
	T_OK,                /**< zone is ok */
	T_ERR                /**< some error occurred while reading the state of the zone */
} thermal_state_t;

/**
 * \struct charge_state_t
 * \brief charge state of battery
 */
typedef enum {
	C_CHARGE,     /**< battery is currently charging */
	C_DISCHARGE,  /**< battery is currently discharging */
	C_CHARGED,    /**< battery is charged */
	C_NOINFO,     /**< hardware doesn't give information about the state */
	C_ERR         /**< some error occurred while reading the charge state */
} charge_state_t;

/**
 * \struct batt_state_t
 * \brief battery life status
 */
typedef enum {
	B_HIGH,       /**< remaining battery life is high */
	B_MED,        /**< remaining battery life is medium */
	B_LOW,        /**< remaining battery life is low */
	B_CRIT,       /**< remaining battery life is critical */
	B_HARD_CRIT,  /**< remaining battery life is hard critical, you have a few minutes to charge */
	B_ERR         /**< some error occurred while reading the battery state */
} batt_state_t;

/**
 * \struct thermal_mode_t
 * \brief cooling mode
 */
typedef enum {
	CO_ACT,       /**< fans will be turned after the temperature passes a critical point */
	CO_PASS,      /**< devices will be put in a lower power state after a critical point */
	CO_CRIT,      /**< system goes into suspend to disk if possible after a critical temperature */
	CO_ERR        /**< some error occurred while reading the cooling mode */
} thermal_mode_t;

/**
 * \enum fan_state_t
 * \brief fan states
 */
typedef enum {
	F_ON,         /**< fan is on */
	F_OFF,        /**< fan is off */
	F_ERR         /**< some error occurred with this fan */
} fan_state_t;

/**
 * \struct fan_t
 * \brief fan data
 */
typedef struct {
	char name[MAX_NAME];         /**< name of the fan found in proc vfs */
	char state_file[MAX_NAME];   /**< state file for the fan */
	fan_state_t fan_state;       /**< current status of the found fan */
} fan_t;

/**
 * \struct battery_t
 * \brief information found about battery
 */
typedef struct {
	char name[MAX_NAME];         /**< name of the battery found in proc vfs */
	char state_file[MAX_NAME];   /**< corresponding state file name + path */
	char info_file[MAX_NAME];    /**< corresponding info file + path */
	char alarm_file[MAX_NAME];   /**< corresponding alarm file + path */
	int present;                 /**< battery slot is currently used by a battery or not? 0 if not, 1 if yes */
	int design_cap;              /**< assuming capacity in mAh*/
	int last_full_cap;           /**< last full capacity when the battery was fully charged */
	int design_voltage;          /**< design voltage in mV */
	int present_rate;            /**< present rate consuming the battery */
	int remaining_cap;           /**< remaining capacity, used to calculate percentage */
	int present_voltage;         /**< present voltage */
	int design_warn;             /**< specifies how many mAh need to be left to have a hardware warning */
	int design_low;              /**< specifies how many mAh need to be left before the battery is low */
	int design_level1;           /**< capacity granularity 1 */
	int design_level2;           /**< capacity granularity 2 */
	int alarm;                   /**< generate hardware alarm in alarm "units" */
	/* calculated states */
	int percentage;              /**< remaining battery percentage */
	int charge_time;             /**< remaining time to fully charge the battery in minutes */
	int remaining_time;          /**< remaining battery life time in minutes */

	/* state info */
	charge_state_t charge_state; /**< charge state of battery */
	batt_state_t batt_state;     /**< battery capacity state */
} battery_t;

/**
 * \struct thermal_t
 * \brief information about thermal zone
 */
typedef struct {
	char name[MAX_NAME];          /**< name of the thermal zone */
	int temperature;              /**< current temperature of the zone */
	int frequency;                /**< polling frequency for this zone */
	char state_file[MAX_NAME];    /**< state file + path of the zone */
	char cooling_file[MAX_NAME];  /**< cooling mode file + path */
	char freq_file[MAX_NAME];     /**< polling frequency file + path */
	char trips_file[MAX_NAME];    /**< trip points file + path */
	char temp_file[MAX_NAME];     /**< temperature file + path */
	thermal_mode_t therm_mode;    /**< current cooling mode */
	thermal_state_t therm_state;  /**< current thermal state */
} thermal_t;

/**
 * \struct adapter_t
 * \brief information about ac adapater
 */
typedef struct {
	char *name;                   /**< ac adapter name */
	char state_file[MAX_NAME];    /**< state file for adapter + path */
	power_state_t ac_state;       /**< current ac state, on-line or off-line */
} adapter_t;

/**
 * \struct global_t
 * \brief global acpi structure
 */
typedef struct {
	int batt_count;               /**< number of found batteries */
	int thermal_count;            /**< number of found thermal zones */
	int fan_count;                /**< number of found fans */
	int temperature;              /**< system temperature if we only have on thermal zone */
	adapter_t adapt;              /**< ac adapter */
} global_t;

/**
 * Array for existing batteries, loop until
 * globals->battery_count
 */
battery_t batteries[MAX_ITEMS];
/**
 * Array for existing thermal zones, loop until
 * globals->thermal_count
 */
thermal_t thermals[MAX_ITEMS];
/**
 * Array for existing fans, loop until
 * globals->fan_count
 */
fan_t fans[MAX_ITEMS];
/**
 * Finds existing batteries and fills the
 * corresponding batteries structures with the paths
 * of the important to parse files
 * @param globals pointer to global acpi structure
 */
int init_acpi_batt(global_t *globals);
/**
 * Finds existing ac adapter and fills the
 * adapt structure with the paths
 * of the important to parse files
 * @param globals pointer to global acpi structure
 */
int init_acpi_acadapt(global_t *globals);
/**
 * Finds existing thermal zones and fills
 * corresponding thermal structures with the paths
 * of the important to parse files for thermal information
 * @param globals pointer to global acpi structure
 */
int init_acpi_thermal(global_t *globals);
/**
 * Finds existing fans and fills
 * corresponding fan structures with the paths
 * of the important to parse files for fan information
 * @param globals pointer to global acpi structure
 */
int init_acpi_fan(global_t *globals);

/**
 * Checks if the system does support ACPI or not
 * @return SUCCESS if the system supports ACPI or, NOT_SUPPORTED
 */
int check_acpi_support(void);

/**
 * Gathers all information of a given battery and filling
 * a struct with it
 * @param num number of battery
 */
int read_acpi_batt(const int num);
/**
 * Looks up if the ac adapter is plugged in or not
 * and sets the values in a struct
 * @param globals pointer to the global acpi structure
 */
void read_acpi_acstate(global_t *globals);
/**
 * Gathers all information of a given thermal zone
 * and sets the corresponding values in a struct
 * @param num zone
 * @param globals pointer to global acpi struct, needed if there is just one zone
 */
int read_acpi_zone(const int num, global_t *globals);
/**
 * Gathers all information about given fan
 * and sets the corresponding values in a struct
 * @param num number for the fan to read
 */
int read_acpi_fan(const int num);
#endif /* !__LIBACPI_H__ */
