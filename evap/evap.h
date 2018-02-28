/*

  evap.h - Evaluate Parameters 2.3 for C (the getopt et.al. replacement)

  Copyright (C) 1990 - 1995 by Stephen O. Lidie and Lehigh University.
			 All rights reserved.

*/

#define P_PDT_VERSION "2.0"	/* Parameter Description Table version */

#ifndef TRUE
#define TRUE  1			/* TRUE */
#endif
#ifndef FALSE
#define FALSE 0			/* FALSE */
#endif

#define P_TYPE_SWITCH      0	/* type ordinals */
#define P_TYPE_STRING      1
#define P_TYPE_REAL        2
#define P_TYPE_INTEGER     3
#define P_TYPE_BOOLEAN     4
#define P_TYPE_FILE        5
#define P_TYPE_KEY         6
#define P_TYPE_APPLICATION 7
#define P_TYPE_NAME        8
#define P_MAXIMUM_TYPES    9	/* maximum types supported */

#define P_MAX_KEYWORD_LENGTH 31	/* maximum parameter length */
#define P_MAX_KEYWORD_VALUE_LENGTH 256 /* maximum parameter value length */
#define P_MAX_PARAMETER_HELP 256 /* max parameters that can have full_help */
#define P_MAX_PARAMETER_HELP_LENGTH 1024 /* maximum parameter help length */
#define P_MAX_VALID_VALUES 32	/* maximum number of key values */
#define P_MAX_EMBEDDED_COMMANDS 100 /* maximum count of original, pristine, PVTs to keep */
#define P_MAX_EMBEDDED_ARGUMENTS 256 /* maximum count of embedded command arguments */

#define P_HELP 0		/* display command information */

#define P_HHURFL 0		/* Help Hook text, Usage, required_file_list */
#define P_HHUOFL 1		/* Help Hook text, Usage, optional_file_list */
#define P_HHUNFL 2		/* Help Hook text, Usage, no_file_list */
#define P_HHBRFL 3		/* Help Hook text, Brief, required_file_list */
#define P_HHBOFL 4		/* Help Hook text, Brief, optional_file_list */
#define P_HHBNFL 5		/* Help Hook text, Brief, no_file_list */
#define P_HHERFL 6		/* Help Hook text, Error, required_file_list */
#define P_HHENFL 7		/* Help Hook text, Error, no_file_list */
#define P_MAXIMUM_HELP_HOOKS 8	/* maximum Help Hooks available */

struct pdt_header {		/* PDT header */
  char *version;		/* PDT version */
  char *help_module_name;	/* help module */
  char *file_list;		/* trailing file list flag */
};

union evap_value {		/* one evaluate_parameters value */
  short int switch_value;
  char *string_value;
#ifdef nosve
  long double real_value;
#else
  double real_value;
#endif
  int integer_value;
  short int boolean_value;
  char *file_value;
  char *key_value;
  char *application_value;
  char *name_value;
};

typedef union evap_value evap_Value; /* an alias */

struct evap_list_value {	/* a 'list of' value */
  char  *unconverted_value;	/* value before type conversion */
  evap_Value value;		/* type-converted value */
};

typedef struct evap_list_value evap_List_Value;

struct evap_parameter_value {	/* parameter value */
  char  *parameter;		/* official parameter spelling */
  char  *alias;			/* this parameter also known as */
  short int specified;		/* if this paramameter entered by user */
  short int changeable;		/* if parameter is changeable by user */
  short int type;		/* this parameter's type */
  char  *default_variable;	/* default environment variable */
  char  *unconverted_value;	/* value before type conversion */
  char  *description;		/* for usage information */
  int   list_state;		/* 0   = not 'list of',
				   1   = list initialized by genpdt, malloc 10
				         list values and store the first,
				   2++ = push successive command line values
				         and realloc as needed */
  evap_List_Value  *list;	/* pointer to 'list of' values or NULL if not
				   a list parameter */
  char  *valid_values[P_MAX_VALID_VALUES]; /* valid values, last one NULL */
  evap_Value value;		/* parameter value */
};

typedef struct evap_parameter_value evap_Parameter_Value; /* an alias */

struct evap_application_command { /* application command */
  char  *command;		/* official command spelling */
  char  *alias;			/* this command also known as */
  int   (*proc)(int argc,	/* pointer to comand processor */
		char *argv[]);
};

typedef struct evap_application_command evap_Application_Command; /* an alias */

extern char *evap_Type_Strings[]; /* valid types, indexed by type ordinal */

extern char *evap_Help_Hooks[]; /* Help Hooks text, indexed by hook ordinal */

void evap_pac(char *prompt,	/* evaluate_parameters/process_application_commands prototype */
	      FILE *I,
	      evap_Application_Command Application_Commands_Table[]);

#ifdef __cplusplus
extern "C" {
int evap(int *argc,		/* evaluate_parameters prototype */
	 char **argv[],
	 struct pdt_header pdt,
	 void *check_parameters_function,
	 evap_Parameter_Value *pvt);
}
#else
int evap(int *argc,		/* evaluate_parameters prototype */
	 char **argv[],
	 struct pdt_header pdt,
	 void *check_parameters_function,
	 evap_Parameter_Value *pvt);
#endif
