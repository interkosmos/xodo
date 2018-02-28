/*

  evap.c - Evaluate Parameters 2.3 for C (the getopt et.al. replacement)
 
  Copyright (C) 1990 - 1995 by Stephen O. Lidie and Lehigh University.
			 All rights reserved.

*/

#ifdef _AIX
#define _POSIX_SOURCE
#define _ALL_SOURCE
#endif

#ifdef __hpux
#define _POSIX_SOURCE
#define _INCLUDE_XOPEN_SOURCE
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "evap.h"
#include "../pdt/bang_pdt_out"
#include "../pdt/disac_pdt_out"

char   *evap_Type_Strings[P_MAXIMUM_TYPES] = {
  "switch",
  "string",
  "real",
  "integer",
  "boolean",
  "file",
  "key",
  "application",
  "name",
};				/* indexed by type ordinal */

/* Help Hooks that provide some customization of evap's help output. */

char   *evap_X11 = NULL;	/* see evap.c(2), section `Interface to X11 XtAppInitialize' */
char   *evap_Help_Hooks[P_MAXIMUM_HELP_HOOKS] = {
  " file(s)\n",
  " [file(s)]\n",
  "\n",
  "\nfile(s) required by this command\n\n",
  "\n[file(s)] optionally required by this command\n\n",
  "\n",
  "Trailing file name(s) required.\n",
  "Trailing file name(s) not permitted.\n",
};				/* indexed by Help Hook ordinal */
int    evap_embed = 0;		/* 1 IFF embedding Evaluate Parameters */
char   evap_shell[514];		/* user's preferred shell */
evap_Application_Command *evap_commands; /* evap_pac's combined command list */


int    evap_display_message_module(char *message_module_name,
				   char *parameter_help[],
				   int *parameter_help_count,
				   FILE *PAGER);
void   evap_display_usage(char *command,
			  struct pdt_header pdt,
			  evap_Parameter_Value pvt[],
			  FILE *PAGER);
void   evap_display_which(char *command,
			  FILE *PAGER);
void   evap_type_conversion(evap_Parameter_Value *pvte);
int    evap_type_verification(evap_Parameter_Value *pvte,
			      int *error);


int    evap(int *argc,
	    char **argv[],
	    struct pdt_header pdt,
	    void *check_parameters_function,
	    evap_Parameter_Value *pvt)

/*

  Step through the parameters, checking if they are valid, and if they
  require a value that one is specified and that it is valid. Note that
  it changes argv and argc to point to file names that appear AFTER all
  the parameters.  pvt is modified with the new parameter values.
  Because argv[0], the program name, is no longer available after calling
  Evaluate Parameters, stuff it into pvt[P_HELP].unconverted_value.
 
  If a help Message Module is specified in the PDT header, then search the
  Message Module archive file libevapmm.a for the help text and display it
  if the -help switch is specified on the command line.  If the Message
  Module cannot be located then generate a short Usage: report.
 
  The check_parameters_function is currently unused and should be NULL.

 
  For related information see the evap/C header file evap.h.  Complete
  help can be found in the man pages evap(2), evap.c(2), EvaP.pm(2),
  evap.tcl(2) and evap_pac(2).


			   Revision History

  Stephen.O.Lidie@CDC1.CC.Lehigh.EDU, LUCC, 90/12/28. (PDT version 1.0)
    . Original release - evaluate_parameters and generate_pdt.

  Stephen.O.Lidie@CDC1.CC.Lehigh.EDU, LUCC, 91/07/01. (PDT version 1.1)
    . Minor bug fixes in generate_pdt.
    . Add support for the name type.
    . Add the new command display_command_information (disci).
    . Add support for help message modules.
    . Add the new command add_message_modules (addmm).
    . Minor bug fix in evaluate_parameters handling of key type.
    . Improve evaluate_parameters error messages.
    . Provide an install script for VX/VE, EP/IX, SunOS, Stardent and AIX.
    . Add support for PDTEND file_list option.
    . Add -qualifier parameter to generate_pdt so that the PDT/PVT names
      can be qualified for 'multiple entry point' programs.  Similar to
      COMPASS QUAL blocks.

  Stephen.O.Lidie@CDC1.CC.Lehigh.EDU, LUCC, 91/08/01. (PDT version 1.1)
    . Allow -- to end command line parameters.
    . Don't print the colon after the type for disci; generate_pdt supplies
      it for us.  This allows the description to be better customized.
    . When type-checking keywords, first search for an exact match (as is
      currently done) and if that fails, search for a substring match.
    . In a similar manner, when the exact match for a command line parameter
      fails, try a substring match (but not for the alias).
    . If the evaluate_parameters message module is missing, generate a short
      Usage: display.
    . If no parameter alias, use the full spelling for Usage: and -help.

  Stephen.O.Lidie@CDC1.CC.Lehigh.EDU, LUCC, 91/12/02. (PDT version 1.2)
    . Do the 'which' function internally rather than using slow system.
    . exec the ar command to display the message module rather than
      using slow system.
    . If an environment variable is specified as a default value for a
      parameter, the variable is defined and the parameter is not 
      specified, use the value of the environment variable for the
      parameter's value.
 
  lusol@Lehigh.EDU 93/01/02. (PDT version 1.2)  Version 1.4
    . Evaluate parameter values within grave accents, or
      backticks.  Add -usage_help to display_command_information.
      Add support for parameter descriptions in the message module
      file for -full_help.
 
  lusol@Lehigh.EDU 93/02/19. (PDT version 1.2)  Version 1.5
    . Document the interface to X11 Release 4 XGetDefault.
    . For parameters of type File expand $HOME and ~.
    . Make evap.c more ANSI-like - define function prototypes and use returns
      consistently.
    . Make evap.h more ANSI-like - replace the #if .. #endif surrounding the 
      evaluate_parameters documentation with a C comment construct.  However,
      all the C comments embedded in the sample code are now PASCAL comments
      of the form {* ... *}.  Ugh, but that's the price I had to pay to make
      the code portable!
 
  lusol@Lehigh.EDU 93/05/18. (PDT version 1.2)  Version 1.6
    . Expose the -full_help command line parameter in evaluate_parameters.
    . For type boolean parameters allow TRUE/YES/1 and FALSE/NO/0, either
      upper or lower case.
    . These changes make evaluate_parameters for C conform, as much as
      possible, to the initial release of evaluate_parameters for Perl.
 
  lusol@Lehigh.EDU 93/08/20. (PDT version 2.0)  Version 2.0
    . Convert evap/C and friends to ANSI C.
    . Expose the -usage_help command line parameter in evaluate_parameters.
    . Full support for 'list of' command line parameters in evap/C and
      generate_pdt.  The PDT list syntax is identical to Perl's list syntax,
      and, of course, to evap/Perl.
    . PDT lines beginning with # are comments.
 
  lusol@Lehigh.EDU 94/03/29. (PDT version 2.0)  Version 2.1
    . Replace help alias `disci' with `?'.
    . Add ON/OFF as valid boolean values.
    . Don't type-check unspecified and empty default PDT values.
    . Move documentation from evap.h to the man page evap.c(2).
    . Obey MANPAGER (or PAGER) environment variable and page help output.
      Default pager is `more'.  Since this changes the behavior of
      evaluate_parameters, the boolean environment variable D_EVAP_DO_PAGE
      can be set to FALSE/NO/OFF/0, any case, to disable this automatic
      paging.
    . Implement Help Hooks to customize evap's help output.
    . Use only a command's basename for -usage_help.
    . To use evaluate_parameters as an embedded command line processor call
      the internal routine `evap_pac'.
 
  lusol@Lehigh.EDU 94/11/03. (PDT version 2.0)  Version 2.2
 
  lusol@Lehigh.EDU 95/10/27. (PDT version 2.0)  Version 2.3
    . Revert to -h alias rather than -?.  (-? -?? -??? still available.)

*/
{
  char   *arg;
  char   *command;
  char   *default_variable;
  int    error;
  short  full_help;
  short  usage_help;
  char   *getenv();
  int    i, j, entry;
  char   *parameter_help[P_MAX_PARAMETER_HELP];
  int    parameter_help_count;
  char   *parameter_help_name;
  evap_List_Value *lve;
  FILE   *PAGER = NULL;
  char   *pager;
  extern int evap_embed;
  static int opvt_count = 0;
  int    registered;
  
  static struct {
    char *MM;			/* register by Message Module name */
    evap_Parameter_Value *pvt;	/* pointer to original pristine PVT  */
  } opvt[P_MAX_EMBEDDED_COMMANDS]; /* maximum count of embedded PVTs to keep */

  error = 0;			/* no errors */
  full_help = FALSE;		/* assume standard -help (brief) help */
  usage_help = FALSE;		/* assume standard -help (brief) help */

  if (strncmp(pdt.version, "PDT Version 2.0 ", 16) < 0) {
    fprintf( stderr, "Error - Evaluate Parameters has detected an illegal PDT.\n" );
    exit( 1 );
  }

  if (check_parameters_function != NULL) {
    fprintf( stderr, "Informative - check_parameters_function is not supported.\n" );
  }

  if ( evap_embed ) {
    /*
       Initialize for a new call in case Evaluate Parameters is embedded in an application.
       Register new PVT's if required so subsequent embedded calls can restore the incoming
       PVT to it's pristine state.
    */
    registered = 0;
    for ( i=0; i<opvt_count; i++ ) {
      if ( strcmp( pdt.help_module_name, opvt[i].MM ) == 0 ) {
	registered = 1;		/* `i' = registered oridinal */
	break;
      }
    } /* forend */
    if ( registered == 0 ) {
      if ( opvt_count >= P_MAX_EMBEDDED_COMMANDS ) {
	fprintf( stderr, "Too many embedded commands (max=%d), evap results will be unpredictable.\n", P_MAX_EMBEDDED_COMMANDS );
	goto UH_OH;
      }
      for (j=0; (pvt[j].parameter) != NULL; j++); /* count number of PVT entries */
      opvt[opvt_count].MM = pdt.help_module_name;
      opvt[opvt_count].pvt = (evap_Parameter_Value *)malloc( j * sizeof( evap_Parameter_Value ) );
      for (j=0; (pvt[j].parameter) != NULL; j++) {
	opvt[opvt_count].pvt[j] = pvt[j];
      }
      opvt_count++;
    } else {
      for (j=0; (pvt[j].parameter) != NULL; j++) {
	pvt[j] = opvt[i].pvt[j];
      }
    }
  } /* ifend embed */

 UH_OH:
  command = *argv[0];
  pvt[P_HELP].unconverted_value = command;

  while (*argc > 1 && (*argv)[1][0] == '-') {
    (*argc)--;
    arg = *++(*argv);		/* get this parameter */
    arg++;			/* skip over the - */
    if (*arg == '-') goto EVAP_FIN; /* -- ends command line parameters */
    entry = -1;

    if ( (strcmp( arg, "full_help" ) == 0) || (strcmp( arg, "???" ) == 0) ) {
      full_help = TRUE;		/* flag special full help from disci */
      pvt[P_HELP].specified = TRUE;
      continue;			/* while argc */
    }  
    if ( (strcmp( arg, "usage_help" ) == 0) || (strcmp( arg, "??" ) == 0) ) {
      usage_help = TRUE;	/* flag special usage help from disci */
      pvt[P_HELP].specified = TRUE;
      continue;			/* while argc */
    }  
    if ((strcmp( arg, "?" ) == 0) ) {
      pvt[P_HELP].specified = TRUE;
      continue;			/* while argc */
    }  

    for (i=0; (pvt[i].parameter) != NULL && entry == -1; i++) 
      if ( (strcmp(arg, pvt[i].parameter) == 0) && pvt[i].changeable)
        entry = i;		/* exact match on full form */

    for (i=0; (pvt[i].parameter) != NULL && entry == -1; i++)
      if ( (strcmp(arg, pvt[i].alias) == 0) && pvt[i].changeable)
        entry = i;		/* exact match on alias */

    if (entry == -1) {		/* no match so far, try a substring match */
      for (i=0; (pvt[i].parameter) != NULL; i++) 
        if ( (strncmp(arg, pvt[i].parameter, strlen(arg)) == 0) &&
             pvt[i].changeable) {
          if (entry != -1) {
            fprintf( stderr, "Ambiguous parameter: -%s.\n", arg );
	    error = 1;
	    break; /* for */
          }
          entry = i;		/* substring match on full form */
        }
    }

    if ( error == 1 )
      continue; /* while arc */

    if (entry ==  -1) {
      fprintf( stderr, "Invalid parameter: -%s.\n", arg );
      error = 1;
      continue; /* while argc */
    }

    pvt[entry].specified = TRUE; /* mark it as specified by the user */
    if (pvt[entry].type != P_TYPE_SWITCH) { /* value for non-switch types */
      if (*argc > 1) {
        (*argc)--;
        pvt[entry].unconverted_value = *++(*argv);
      } else {
        fprintf( stderr, "Value required for parameter -%s.\n", pvt[entry].parameter );
	error = 1;
      }
    } else {
      pvt[entry].unconverted_value = "TRUE";
    } /* ifend */

    evap_type_verification( &pvt[entry], &error );

  } /* while argc */

  if (pvt[P_HELP].specified) {	/* display command information */

    /* 
       Establish the proper pager and open the pipeline.  Do no paging
       if the boolean environment variable D_EVAP_DO_PAGE is FALSE.
    */

    if ( (pager = getenv( "MANPAGER" )) == NULL || (strcmp( pager, "" ) == 0) ) {
      if ( (pager = getenv( "PAGER" )) == NULL || (strcmp( pager, "" ) == 0) ) {
	pager = "more";
      }
    }
    if ( (default_variable = getenv( "D_EVAP_DO_PAGE" )) != NULL ) {
      if ( strcmp( default_variable, "" ) != 0 ) {
	for ( i = 0; default_variable[i]; i++ ) {
	  if ( islower( default_variable[i]) )
	    default_variable[i] = toupper( default_variable[i] );
	}
	if( (strcmp(default_variable, "FALSE") == 0) || (strcmp(default_variable, "NO") == 0) ||
	    (strcmp(default_variable, "OFF") == 0)   || (strcmp(default_variable, "0") == 0) ) {
	  PAGER = stdout;
	}
      } /* ifend non-NULL D_EVAP_DO_PAGE */
    } /* ifend environment variable D_EVAP_DO_PAGE exists */
    if ( PAGER != stdout ) {
      if ( (PAGER = popen( pager, "w" )) == NULL ) {
	PAGER = stdout;
      }
    }

    if (full_help) {
      fprintf ( PAGER, "Command Source:  " );
      evap_display_which(command, PAGER);
      fprintf ( PAGER, "\nMessage Module Name:  %s\n\n", pdt.help_module_name );
    }

    if ( usage_help || (evap_display_message_module(pdt.help_module_name,
          parameter_help, &parameter_help_count, PAGER) != 0) )
      evap_display_usage(command, pdt, pvt, PAGER);

    fprintf( PAGER, "\nParameters:\n" );
    if ( ! full_help )
      fprintf( PAGER, "\n" );
    for (i=0; (pvt[i].parameter != NULL); i++) {
      /* changeable = -1 means valid but HIDDEN from -disci */
      if (pvt[i].changeable == 1) { /*valid and advertisable*/
	if ( full_help )
	  fprintf( PAGER, "\n" );
	if (strcmp(pvt[i].alias, "") != 0)
          fprintf( PAGER, "-%s, %s%s\n",pvt[i].parameter, pvt[i].alias, pvt[i].description );
        else
          fprintf( PAGER, "-%s%s\n",pvt[i].parameter, pvt[i].description );

	if ( full_help && (parameter_help_count >= 0) ) {
	  fprintf( PAGER, "\n" );
          if ( i == P_HELP ) {
            fprintf( PAGER, "\tDisplay information about this command, which includes\n" );
            fprintf( PAGER, "\ta command description with examples, plus a synopsis of\n" );
	    fprintf( PAGER, "\tthe command line parameters.  If you specify -full_help\n" );
	    fprintf( PAGER, "\trather than -help complete parameter help is displayed\n" );
	    fprintf( PAGER, "\tif it's available.\n" );
          }
	  /*
             Search parameter_help for this parameter's description.
	     Each entry is a list of new-line-separated lines - the
	     first line is the name of the parameter.
          */
	  for ( j=0; j <= parameter_help_count; j++ ) {
	    parameter_help_name = strtok( parameter_help[j], "\n" );
	    if ( strcmp( pvt[i].parameter, parameter_help_name ) == 0 ) {
	      fprintf( PAGER, "%s", parameter_help[j]+strlen(parameter_help_name)+1 );
	    } /* ifend */
	  } /* forend */
	} /* ifend full_help and parameter_help_count >= 0 */
      } /* ifend valid and advertisable */ 
    } /* forend all parameters */

    if ( evap_X11 != NULL )
      fprintf( PAGER, "%s", evap_X11 );

    if (strcmp(pdt.file_list, "no_file_list") == 0) {
      fprintf( PAGER, "%s", evap_Help_Hooks[P_HHBNFL] );
    } else if (strcmp(pdt.file_list, "required_file_list") == 0) {
      fprintf( PAGER, "%s", evap_Help_Hooks[P_HHBRFL] );
    } else {
      fprintf( PAGER, "%s", evap_Help_Hooks[P_HHBOFL] );
    }

    if ( PAGER != stdout ) {
      pclose( PAGER );
    }

    if ( evap_embed ) {
      return( -1 );
    } else {
      exit( 0 );
    }

  } /* ifend help specified */

EVAP_FIN:
  /*
    Now loop through the PDT looking for unspecified parameters.

    If there is a default environment variable specified and it is
    defined then update the structure member unconverted_value.

    Complain if there are any $required parameters.

    Then perform type verification followed by type conversion.
  */

  for (i = 0; pvt[i].parameter != NULL; i++) {

    if ( i == P_HELP ) continue; /* for */

    if (! pvt[i].specified) {

      if (strcmp("$required", pvt[i].unconverted_value) == 0) {
	fprintf( stderr, "Parameter %s is required but was omitted.\n", pvt[i].parameter );
	error = 1;
	continue;
      }

      if (pvt[i].default_variable != NULL) {
	if ((default_variable = getenv(pvt[i].default_variable)) != NULL) {
	  pvt[i].unconverted_value = default_variable;
	} /* ifend */
      } /* ifend default variable */ 

      if ( pvt[i].list == NULL && (strcmp( pvt[i].unconverted_value, "" ) != 0) ) {
	evap_type_verification( &pvt[i], &error );
      } else if ( pvt[i].list != NULL ) {
	lve = pvt[i].list;
	while ( lve->unconverted_value != NULL ) {
	  pvt[i].unconverted_value = lve->unconverted_value;
	  if ( evap_type_verification( &pvt[i], &error ) )
	    lve->unconverted_value = pvt[i].unconverted_value;
	  pvt[i].list_state++;	/* update state for evap_type_conversion */
	  lve++;
	} /* whilend */
      } /* ifend 'list of' parameter */

    } /* ifend parameter not specified */

    if ( error == 0 )
      evap_type_conversion( &pvt[i] );

  } /* forend */

  if ((strcmp(pdt.file_list, "no_file_list") == 0) && (*argc > 1)) {
    fprintf( stderr, "%s", evap_Help_Hooks[P_HHENFL] );
    error = 1;
  } 
  if ((strcmp(pdt.file_list, "required_file_list") == 0) && (*argc == 1)) {
    fprintf( stderr, "%s", evap_Help_Hooks[P_HHERFL] );
    error = 1;
  }

  if ( error != 0 ) {
    fprintf( stderr, "Type %s -h for command line parameter information.\n", pvt[P_HELP].unconverted_value );
  }

  if ( error != 0 && ! evap_embed ) { exit( 1 ); } /* command line parsing failed, error */
  if ( ! error ) {
    return( 0 );		/* return False for success.... sigh */
  } else {
    return( 1 );		/* return True for failure... stupid C/Unix */
  }

} /* end Evaluate Parameters */




int    evap_type_verification(evap_Parameter_Value *pvte, int *error)

/*
  Type-check/initialize the unconverted_value member (a C string).
  Evaluate backticked items.

  Lists are a little weird as they may already have default values from the
  PDT declaration. The first time a list parameter is specified on the
  command line we must first empty the list of its default values.  The
  list_state flag thus can be in one of two states: 1 = the list has
  possible default values from the PDT - malloc space for 10 list values and
  store the first at the head of the list, and 2++ = from now just keep pushing
  new command line values on the list, reallocing as needed.

  Return 1 IFF we have changed the original unconverted value, since we might
  need to update a list.  Also be VERY careful to malloc new space so we don't
  end up trying to write in a read only segment!
*/
{

#define BACKMAX 1024

  char   *backcomd;
  int    backcnt;
  int    backincr;
  char   *backline;
  char   *backptr;
  FILE   *backtick;
  int    i;
  int    valid;
  char   *value;
  char   home[6];
  struct passwd *pwent;
  char   *home_ptr;
  int    rc;

  rc = 0;			/* assume value is not updated */

  if ( pvte->unconverted_value[0] == '`' &&
      pvte->unconverted_value[strlen( pvte->unconverted_value ) - 1] == '`' ) {
    backcomd = (char *)malloc( 10 + strlen( pvte->unconverted_value ) );
    sprintf( backcomd, "echo %s", pvte->unconverted_value );
    if ( (backtick = popen( backcomd, "r" )) != NULL ) {
      backcnt = 1;
      backline = (char *)malloc( BACKMAX * backcnt );
      backptr = fgets( backline, BACKMAX, backtick );
      pvte->unconverted_value = backline;
      while ( backptr != NULL ) {
	backline = (char *) realloc( backline, BACKMAX * ++backcnt );
	pvte->unconverted_value = backline;
	backincr = (BACKMAX * (backcnt - 1)) - (backcnt - 1);
	backptr = fgets( backline+backincr, BACKMAX, backtick );
      }
      pvte->unconverted_value[strlen( pvte->unconverted_value ) - 1] = '\0';
      pclose( backtick );
      free( backcomd );
      rc = 1;
    } /* ifend backtick evaluation */
  } /* backticks */

  value = pvte->unconverted_value; /* get value from pvt entry */

  switch (pvte->type) {

  case P_TYPE_SWITCH:
    break; /* only TRUE or FALSE, guaranteed */

  case P_TYPE_STRING:
    break; /* anything OK */

  case P_TYPE_REAL:
    if (FALSE) {		/* no type enforcment (no U*X consistency either) */
      fprintf( stderr, "Expecting real reference, found \"%s\" for parameter -%s.\n", pvte->unconverted_value, pvte->parameter );
      *error = 1;
      break; /* case */
    }
    break;

  case P_TYPE_INTEGER:
    if((isdigit(*value)) || (value[0] == '+') || (value[0] == '-'))
      value++;
    else {
      fprintf( stderr, "Expecting integer reference, found \"%s\" for parameter -%s.\n",
	      pvte->unconverted_value, pvte->parameter );
      *error = 1;
      break; /* case */
    }
    for(; *value != '\0'; value++) {
      if(! isdigit(*value)) {
        fprintf( stderr, "Expecting integer reference, found \"%s\" for parameter -%s.\n",
		pvte->unconverted_value, pvte->parameter );
	*error = 1;
	break; /* case */
      }
    }
    break;

  case P_TYPE_BOOLEAN:
    pvte->unconverted_value = (char *)malloc( strlen( value ) + 1 );
    strcpy( pvte->unconverted_value, value );
    for ( i = 0; pvte->unconverted_value[i]; i++ ) {
      if ( islower( pvte->unconverted_value[i]) )
        pvte->unconverted_value[i] = toupper( pvte->unconverted_value[i] );
    }
    value = pvte->unconverted_value;
    rc = 1;
    if((strcmp(value, "TRUE") != 0) && (strcmp(value, "FALSE") != 0) &&
       (strcmp(value, "YES") != 0) && (strcmp(value, "NO") != 0) &&
       (strcmp(value, "ON") != 0) && (strcmp(value, "OFF") != 0) &&
       (strcmp(value, "1") != 0) && (strcmp(value, "0") != 0)) {
      fprintf( stderr, "Expecting boolean reference, found \"%s\" for parameter -%s.\n",
	      pvte->unconverted_value, pvte->parameter );
      *error = 1;
      break; /* case */
    }
    break;

  case P_TYPE_FILE:
    home_ptr = NULL;
    strncpy( home, value, 5);	/* look for $HOME or ~ */
    home[5] = '\0';
    if ( strcmp( home, "$HOME" ) == 0) 
      home_ptr = value+5;
    if ( *value == '~')
      home_ptr = value+1;
    if ( home_ptr != NULL ) {	/* expand the shorthand */
      if ( (pwent = getpwuid( getuid() )) != NULL ) {
        pvte->unconverted_value = (char *)malloc( strlen( value ) + strlen( pwent->pw_dir ) );
	strcpy( pvte->unconverted_value, pwent->pw_dir );
	strcat( pvte->unconverted_value, home_ptr );
	value = pvte->unconverted_value;
      } /* ifend pwent */
    } /* ifend expand $HOME */
    rc = 1;
    if (strlen(value) > 255) {
      fprintf( stderr, "Expecting file reference, found \"%s\" for parameter -%s.\n", pvte->unconverted_value, pvte->parameter );
      *error = 1;
      break; /* case */
    }
    break;

  case P_TYPE_KEY:
    if (pvte->valid_values[0]) {

      for(valid=FALSE, i=0; pvte->valid_values[i] && ! valid; i++)
        if (strcmp(value, pvte->valid_values[i]) == 0)
          valid = TRUE;		/* ok, got one */

      if (! valid)		/* no exact match, try a substring match */
        for(valid=FALSE, i=0; pvte->valid_values[i]; i++)
          if (strncmp(value, pvte->valid_values[i], strlen(value)) == 0) {
            if (valid) {
              fprintf( stderr, "Ambiguous keyword for parameter -%s: %s.\n", pvte->parameter, value );
	      *error = 1;
	      break; /* case */
            }
            valid = TRUE;	/* ok, got one */
            pvte->unconverted_value = pvte->valid_values[i];
	    rc = 1;
          } /* ifend update value in pvt */

      if (! valid) {		/* no matches */
        fprintf( stderr, "\"%s\" is not a valid value for the parameter -%s.\n", pvte->unconverted_value, pvte->parameter );
	*error = 1;
	break; /* case */
      }
    }
    break;

  case P_TYPE_APPLICATION:
    break;			/* anything is valid in this case */

  case P_TYPE_NAME:
    for ( ; *value != '\0'; value++ ) {
      if ( *value == ' ' || *value == '\n' || *value == '\t' ) {
	fprintf( stderr, "Expecting name reference, found \"%s\" for parameter -%s.\n",
		pvte->unconverted_value, pvte->parameter );
	*error = 1;
	break; /* case */
      }
    }
    break;

  default:
    fprintf( stderr, "Error in evap_type_verification (default), please get help!\n" );
    exit( 1 );
    break;
  }

  if ( (pvte->list != NULL) && pvte->specified ) {

    /*
       If list_state = 1 malloc room for the first 10 command line values.
       list_state values >= 2 determine when to realloc - it's the count of
       list elements - 1.
    */

    switch ( pvte->list_state ) {

    case 1:			/* store first list value */
      pvte->list = (evap_List_Value *)malloc( 1 * 10 * sizeof( evap_List_Value) );
      pvte->list->unconverted_value = pvte->unconverted_value;
      pvte->list[pvte->list_state++].unconverted_value = NULL;
      break;

    default:			/* push value on the list, realloc as needed */
      if ( ( pvte->list_state + 1 ) % 10 == 0 ) {
	pvte->list =
	  (evap_List_Value *)realloc(pvte->list, ( ( ( pvte->list_state + 1 ) / 10) + 1 ) * 10 * sizeof( evap_List_Value) );
      } /* ifend time to realloc */
      pvte->list[pvte->list_state-1].unconverted_value = pvte->unconverted_value;
      pvte->list[pvte->list_state++].unconverted_value = NULL;
      break;

    } /* casend list_state */

  } /* ifend 'list of' parameter */

  return rc;

} /* end evap_type_verification */




void   evap_type_conversion(evap_Parameter_Value *pvte)

/*
  Convert all the unconverted values, which are C strings, in a type-dependent
  manner, and store the result in the value variant record (union).
*/
{

  char   *value;

  do {				/* for all values */

      if ( pvte->list == NULL ) { /* get scalar value */
	value = pvte->unconverted_value;
      } else {			/* get list value */
	pvte->list_state--;
	value = pvte->list[pvte->list_state - 1].unconverted_value;
      }

    switch (pvte->type) {

    case P_TYPE_SWITCH:
      if (strcmp(value, "TRUE") == 0)
	pvte->value.switch_value = TRUE;
      else
	pvte->value.switch_value = FALSE;
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.switch_value = pvte->value.switch_value;
      break;

    case P_TYPE_STRING:
      pvte->value.string_value = value;
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.string_value = pvte->value.string_value;
      break;

    case P_TYPE_REAL:
      sscanf(value, "%lg", &pvte->value.real_value);
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.real_value = pvte->value.real_value;
      break;

    case P_TYPE_INTEGER:
      sscanf(value, "%d", &pvte->value.integer_value);
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.integer_value = pvte->value.integer_value;
      break;

    case P_TYPE_BOOLEAN:
      if ( (strcmp(value, "TRUE") == 0) || (strcmp(value, "YES") == 0) ||
	  (strcmp(value, "ON") == 0)    || (strcmp(value, "1") == 0) )
	pvte->value.boolean_value = TRUE;
      else
	pvte->value.boolean_value = FALSE;
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.boolean_value = pvte->value.boolean_value;
      break;

    case P_TYPE_FILE:
      pvte->value.file_value = value;
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.file_value = pvte->value.file_value;
      break;

    case P_TYPE_KEY:
      pvte->value.key_value = value;
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.key_value = pvte->value.key_value;
      break;

    case P_TYPE_APPLICATION:
      pvte->value.application_value = value;
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.application_value = pvte->value.application_value;
      break;

    case P_TYPE_NAME:
      pvte->value.name_value = value;
      if ( pvte->list != NULL )
	pvte->list[pvte->list_state - 1].value.name_value = pvte->value.name_value;
      break;

    default:
      fprintf( stderr, "Error in evap_type_conversion (default), please get help!\n" );
      exit( 1 );
      break;
    }

  } while ( pvte->list_state > 1 );

  return;

} /* end evap_type_conversion */



int    evap_display_message_module(char *message_module_name,
				   char *parameter_help[],
				   int *parameter_help_count,
				   FILE *PAGER)

/*
  Search an Evaluate Parameters Message Module file for the specified help
  module and, if found, display it.
 
  The message_module_name parameter is either a simple Message Module name or
  a Message Module name prefixed with the path name of a local Message Module
  archive file.  For example:
 
    disci.mm               names a Message Module disci.mm in file libevapmm.a
    my_mm/my_program.mm    names a Message Module my_progam.mm in file my_mm
*/
{

  FILE*  a;			/* stream to read ar output from */
  char   afn[256];		/* evap `ar' Message Module path name */
  char   ar_str[600];		/* ar p libevapmm.a mm */
  char*  cp;			/* character pointer */
  short  parameter_help_in_progress;
  char   mmn[256];


  if (message_module_name == NULL)
    return( 1 );		/* if no Message Module specified */

  strncpy( mmn, message_module_name, 255 ); /* make local copy */
  cp = strrchr(mmn, '/');	/* check for local archive path */
  if (cp == NULL) {		/* simple Message Module name */
    strcpy(afn, P_EVAP_MM_PATH); /* use default ar file name */
  } else {			/* private Message Module `ar' file */
    *cp = '\0';			/* separate file path name and Message Module name */
    strcpy(afn, mmn);
    strcpy(mmn, cp+1);
  } /* ifend simple Message Module */

  sprintf(ar_str, "ar p %s %s 2> /dev/null", afn, mmn);

  *parameter_help_count = -1;	/* no parameters have help so far */
  parameter_help_in_progress = FALSE; /* not collecting parameter help */

  if ( (a = popen( ar_str, "r" )) == NULL )
    return( 1 );		/* do usage */

  if ( fgets( ar_str, 512, a ) == NULL )
    return( 1 );		/* do usage */

  do {				/* copy Message Module until EOF */
    if ( ar_str[0] == '.' ) {	/* or until start of parameter help */
      parameter_help_in_progress = TRUE;
      /*
	Create an array of parameter help: the first line is the parameter
	name and successive lines are the associated help text.
      */
      (*parameter_help_count)++;
      parameter_help[*parameter_help_count] = (char *)malloc( P_MAX_PARAMETER_HELP_LENGTH );
      strcpy( parameter_help[*parameter_help_count], ar_str+1 );
      continue; /* do */
    } /* ifend start of parameter help */
    if ( parameter_help_in_progress ) {
      if ( strlen( parameter_help[*parameter_help_count] ) + strlen( ar_str ) < P_MAX_PARAMETER_HELP_LENGTH ) {
	strcat( parameter_help[*parameter_help_count], ar_str );
      } else {
	sprintf( ar_str, "\n*** Parameter Help Text Exceeds %d Characters ***\n", P_MAX_PARAMETER_HELP_LENGTH );
	strcpy( parameter_help[*parameter_help_count] + P_MAX_PARAMETER_HELP_LENGTH - strlen( ar_str ) - 1, ar_str );
      }
    } else {
      fprintf( PAGER, "%s", ar_str );
    }
  } while (fgets( ar_str, 512, a) != NULL); /* doend */
  pclose( a );

  return( 0 );			/* success */

} /* end evap_display_message_module */





void  evap_display_usage(char *command,
			 struct pdt_header pdt,
			 evap_Parameter_Value pvt[],
			 FILE *PAGER)

/*
  Generate a 'standard' usage display in lieu of a help Message Module.
*/
{

  int    i;
  int    optional_param_count;
  char   *basename;

  optional_param_count = 0;

  if ( (basename = strrchr( command, '/' )) == NULL ) { /* only basename for usage help */
    basename = command;
  } else {
    basename++;
  }
  fprintf( PAGER, "\nUsage: %s", basename );

  for(i=0; pvt[i].parameter != NULL; i++) {
    if (pvt[i].changeable == 1) { /* valid and advertisable */
      optional_param_count++;
      if (strcmp(pvt[i].unconverted_value, "$required") == 0) {
        optional_param_count--;
        if (strcmp(pvt[i].alias, "") != 0)
          fprintf( PAGER, " -%s", pvt[i].alias );
        else
          fprintf( PAGER, " -%s", pvt[i].parameter );
      } /* ifend */
    } /* ifend */
  } /* forend */

  if (optional_param_count > 0) {
    fprintf( PAGER, " [" );
    for(i=0; pvt[i].parameter != NULL; i++) {
      if (pvt[i].changeable == 1) { /* if advertisable */
        if (strcmp(pvt[i].unconverted_value, "$required") != 0) {
          if (strcmp(pvt[i].alias, "") != 0)
            fprintf( PAGER, " -%s", pvt[i].alias );
          else
            fprintf( PAGER, " -%s", pvt[i].parameter );
        } /* ifend */
      } /* ifend */
    } /* forend */
    fprintf( PAGER, "]" );
  } /* ifend */

  if (strcmp(pdt.file_list, "required_file_list") == 0) {
    fprintf( PAGER, "%s", evap_Help_Hooks[P_HHURFL] );
  } else if (strcmp(pdt.file_list, "optional_file_list") == 0) {
    fprintf( PAGER, "%s", evap_Help_Hooks[P_HHUOFL] );
  } else {
    fprintf( PAGER, "%s", evap_Help_Hooks[P_HHUNFL] );
  }

  return;

} /* end evap_display_usage */




void  evap_display_which(char *command,
			 FILE *PAGER)

/*
  Perform a fast 'which' function to show the source of this command.
  (Copied from which.c on UUNET.UU.NET, but with a few bug fixes.)
*/
{

  char   *getenv(), *path = getenv("PATH");
  char   test[1000], *pc, save;
  int    len, found;

  char   name[256];		/* name exclusive of path */
  char   *cp;			/* character pointer */  

  cp = strrchr(command, '/');	/* look for path separator */
  if (cp == NULL)
    strcpy(name, command);	/* if a simple file name */
  else {
    fprintf( PAGER, "%s\n", command );	/* must be an explicit path */
    return;
  }

  pc = path;
  found = 0;

  while (*pc != '\0' && found == 0) {
    len = 0;

    while (*pc != ':' && *pc != '\0') {
      len++;
      pc++;
    } /* whilend */

    save = *pc;
    *pc = '\0';
    sprintf(test, "%s/%s", pc-len, name);
    *pc = save;
    if (*pc)
      pc++;

    found = (0 == access(test, 01)); /* executable */
    if (found) {
      fputs( test, PAGER );
      fputs( "\n", PAGER );
    }

  } /* whilend */

  return;

} /* end evap_display_which */





void evap_pac(char *prompt,
	      FILE *I,
	      evap_Application_Command user_commands[])

/*
   Process Application Commands

   An application command can be envoked by entering either its full spelling or the alias.
*/
{

  int argc;			/* argument count */
  char *argv[P_MAX_EMBEDDED_ARGUMENTS];	/* argument vector */
  evap_Application_Command *cmd_e; /* command entry */
  evap_Application_Command *cmd_p; /* command pointer */
  char *char_p;			/* character pointer */
  int evap_bang_proc(int argc,	/* bang processor */
		     char *argv[]);
  int evap_disac_proc(int argc,	/* display_application_commands processor */
		      char *argv[]);
  char *evap_get_token(char *char_p, int bang ); /* get next token from input line */
  extern evap_Application_Command *evap_commands; /* combined user/evap command list */
  evap_Application_Command evap_commands0[] = {	/* evap_pac special commands */
    {"display_application_commands", "disac", evap_disac_proc},
    {"!", "", evap_bang_proc},
  };				/* user commands MUST be NULL terminated */
  extern int  evap_embed;	/* 1 IFF evap is embedded */
  extern char evap_shell[];	/* user's preferred shell */
  int evap_sorac_proc();	/* sort application commands processor */
  char *getenv();		/* get value of environment variable */
  int i;			/* index */
  char line[514];		/* user's input line */
  char new_line[514];		/* copy of user's input line for bang processor */

  evap_embed = 1;		/* flag to Evaluate Parameters */
  if ( (char_p = getenv( "SHELL" )) == NULL || (strcmp( char_p, "" ) == 0) ) {
    char_p = "/bin/sh";
  }
  strcpy( evap_shell, char_p );	/* save user's preferred shell */

  /* First, combine the special evap_pac commands and the user commands into a new, sorted, command list. */

  i = sizeof(evap_commands0) / sizeof(evap_Application_Command) + 1;
  cmd_p = user_commands;
  while ( cmd_p->command != NULL ) {
    i++;			/* count user commands too */
    cmd_p++;
  }
  if ( (evap_commands = (evap_Application_Command *)malloc( i * sizeof(evap_Application_Command) )) == NULL ) {
    fprintf( stderr, "malloc error requesting space for evap_pac's command list.\n" );
    return;
  }
  for ( i=0; i < sizeof(evap_commands0) / sizeof(evap_Application_Command); i++ ) {
    evap_commands[i] = evap_commands0[i]; /* copy special Evaluate Parameters/process_application_commands commands */
  }
  for ( cmd_p=user_commands; cmd_p->command != NULL; cmd_p++, i++ ) {
    evap_commands[i] = *cmd_p;	/* append user commands */
  }
  qsort( evap_commands, i, sizeof( evap_Application_Command ), evap_sorac_proc );
  evap_commands[i].command = NULL;	/* mark end of PAC command list */

  fprintf( stdout, "%s", prompt );

  while ( fgets( line, 512, I ) != NULL ) {

    argc = 0 ;
    if ( line[0] == '\n' ) { goto GET_USER_INPUT; } /* ignore empty input lines */

    /*
       Span leading whitespace and inspect first character.  If no token get another input
       line.  If a ! ensure a space follows the bang character.  Then get the first token.
    */
    line[strlen( line )-1] = '\0';		/* zap newline */
    for ( char_p=line; *char_p != '\0' && (*char_p == ' ' || *char_p == '\n' || *char_p == '\t'); char_p++ );
    if ( *char_p == '\0' ) goto GET_USER_INPUT;
    if ( *char_p == '!' ) {
      strcpy( new_line, "! " );
      strcat( new_line, ++char_p );	/* ensure a space after the bang */
      char_p = new_line;
    }
    argv[argc++] = evap_get_token( char_p, FALSE ); /* get application command */

    cmd_e = NULL;
    for ( cmd_p=evap_commands; (cmd_p->command != NULL && cmd_e == NULL); cmd_p++ ) {
      if ( strcmp( cmd_p->command, argv[0] ) == 0 ) { cmd_e = cmd_p; }
    }
    for ( cmd_p=evap_commands; (cmd_p->command != NULL && cmd_e == NULL); cmd_p++ ) {
      if ( strcmp( cmd_p->alias, argv[0] ) == 0 ) { cmd_e = cmd_p; }
    }

    if ( cmd_e == NULL ) {
      fprintf( stderr, "Error - unknown command `%s'.  Type `disac -do f' for a\n", argv[0] );
      fprintf( stderr, "list of valid application commands.  You can then ...\n\n" );
      fprintf( stderr, "Type `xyzzy -h' for help on application command `xyzzy'.\n" );
      goto GET_USER_INPUT;
    }

    if ( strcmp( argv[0], "!" ) == 0 ) { /* rest of line is token */
      argv[argc++] = evap_get_token( NULL, TRUE );
      argv[argc] = NULL;
    } else {
      do {			/* get shell-like tokens */
	if ( argc >= P_MAX_EMBEDDED_ARGUMENTS ) {
	  fprintf( stderr, "Too many command arguments (max=%d); please re-enter command.\n", P_MAX_EMBEDDED_ARGUMENTS );
	  goto GET_USER_INPUT;
	}
      } while ( (argv[argc++] = evap_get_token( NULL, FALSE )) != NULL );
      argc--;
    }

    cmd_e->proc( argc, argv );	/* call the evap/usr procedure */

  GET_USER_INPUT:
    fprintf( stdout, "%s", prompt );

  } /* whilend */

  free( evap_commands );
  if (strcmp(prompt, "") != 0) {
    fprintf( stdout, "\n" );
  }

} /* end evap_pac */




int evap_bang_proc(int argc,
		   char *argv[])
{

  char cmd[514];
  extern char evap_shell[];
  
  evap_Help_Hooks[P_HHUOFL] = " Command(s)\n";
  evap_Help_Hooks[P_HHBOFL] = "\nA list of shell Commands.\n\n";
  if ( *argv[1] == '\n' ) { return( 0 ); }
  if ( evap( &argc, &argv, bang_pkg_pdt, NULL, bang_pkg_pvt ) != 0 ) { return( 0 ); }

  sprintf( cmd, "%s -c '%s'", evap_shell, argv[1] );
  system( cmd );

  return( 0 );

} /* end evap_bang_proc */




int evap_disac_proc(int argc,
		    char *argv[])
{

  evap_Application_Command *cmd_p;
  extern evap_Application_Command *evap_commands;

  if ( evap( &argc, &argv, disac_pkg_pdt, NULL, disac_pkg_pvt ) != 0 ) { return( 0 ); }

  if ( strcmp( disac_pkg_pvt[disac_pkg_P_display_option].value.key_value, "full" ) == 0 ) {
    fprintf( stdout, "\nFor help on any application command (or alias) use the -h switch.  For example,\n" );
    fprintf( stdout, "try `disac -h' for help on `display_application_commands'.\n" );
    fprintf( stdout, "\nCommand and alias list for this application:\n\n" );
  }
  for( cmd_p=evap_commands; cmd_p->command != NULL; cmd_p++ ) {
    if ( strcmp( disac_pkg_pvt[disac_pkg_P_display_option].value.key_value, "full" ) == 0 ) {
      fprintf( stdout, "  %s%s%s\n", cmd_p->command, (strcmp( cmd_p->alias, "" ) == 0) ? "" : ", ", cmd_p->alias );

    } else {
      fprintf( stdout, "%s\n", cmd_p->command );
    }
  }
  

  return( 0 );

} /* end evap_disac_proc */




int evap_sorac_proc(const evap_Application_Command *keyval, /* sort application commands processor */
		    const evap_Application_Command *base)
{

  return( strcmp( keyval->command, base->command ) );

} /* end evap_sorac_proc */




char *evap_get_token( char *char_p, int bang ) /* get next token from input line */
{
  
  int str = FALSE;
  static char *token_p;
  char *tp;

  if ( char_p != NULL ) {
    token_p = char_p;		/* initialize token pointer */
  }
  tp = token_p;

  if ( bang ) { return( tp ); }	/* entire line is bang's token */

  if ( *token_p == '"' || *token_p == '\'' ) {
    str = TRUE;
    tp++;			/* do not return leading quote */
  }

  do {
    for ( ; *token_p != '\0' && (*token_p != ' ' && *token_p != '\n' && *token_p != '\t'); token_p++ ); /* span token */
    if ( *(token_p-1) == '"' || *(token_p-1) == '\'' ) {
      str = FALSE;
      *(token_p-1) = '\0';	/* do not return trailing quote */
    }
    if ( str ) { token_p++; }
  } while ( str && *token_p != '\0' );
  *token_p++ = '\0';		/* terminate token */
  for ( ; *token_p != '\0' && (*token_p == ' ' || *token_p == '\n' || *token_p == '\t'); token_p++ ); /* span whitespace */

  if ( *tp == '\0' ) { tp = NULL; }
  return( tp );

} /* end evap_get_token */
