#include <unitest/unitest.h>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <setjmp.h>

test_t this;

static void test_finish( void ) {

  test_suite( NULL );

  if ( this.orc.captured.unlink ) {
    unlink( this.orc.captured.file );
  }

  if ( this.summary.file ) {
    if ( this.env.verbose ) {
      fprintf( this.summary.file,
               " unit::%-14s\t%20s\t%5u\t%6u\t%10u\t%6u\t%7u\t%10u\n\n",
               this.env.prog_name    , "All tests",
               this.total.total      , this.total.passed,
               this.total.segfaulted , this.total.aborted,
               this.total.interrupted, this.total.timed_out );
    }
    else {
      fprintf( this.summary.file,
               " unit::%-14s\t%20s\t%5u\t%6u\t%10u\t%6u\n\n",
               this.env.prog_name    , "All tests",
               this.total.total      , this.total.passed,
               this.total.segfaulted , this.total.aborted );
    }

    fclose( this.summary.file );
  }

  if ( this.total.untested ) {
    TEST_STYLE_SKIPPED();
    fprintf( this.stderr_orig, "[UniTest] Warning: Skipped %u tests due to debugging.\n", this.total.untested );
    STYLE_RESET( this.stderr_orig );
  }

  exit( this.total.total - this.total.passed < 256 ? this.total.total - this.total.passed : 255 );
}

/*
  Adapted from:
  https://github.com/ThrowTheSwitch/Unity
  MIT License, Copyright (c) <year> 2007-21
  by Mike Karlesky, Mark VanderVoord and Greg Williams.
 */

#define SIG_CASE_PRINT( sig, msg )                              \
  if ( this.env.verbose ) {                                     \
    fprintf( this.stderr_orig, "\n" );                          \
  }                                                             \
  TEST_STYLE_ERROR();                                           \
  if ( !strcmp( sig, "SIGSEGV" ) )                              \
    STYLE( this.stderr_orig, 196, ST_BLINK );                   \
  fprintf( this.stderr_orig, " *** Caught %s [%s] ***",         \
           sig, msg );                                          \
  STYLE_RESET( this.stderr_orig );                              \
  fprintf( this.stderr_orig, " from " );                        \
  TEST_STYLE_EMPH_LIGHT();                                      \
  fprintf( this.stderr_orig, "%s:%s:%d", this.env.prog_name,    \
           this.env.func, this.env.line );                      \
  STYLE_RESET( this.stderr_orig );                              \
  fprintf( this.stderr_orig, "." )


#define SIG_SUB_CASE__( sig, sub, msg ) case sub :			\
  SIG_CASE_PRINT( sig, msg );                               \
  break

#define SIG_SUB_CASE( sig, sub, msg ) SIG_SUB_CASE__( #sig, sub, msg )
#define SIG_CASE( signal, msg )       SIG_SUB_CASE__( #signal, signal, msg )

#define HANDLE_SIGNAL( signal ) do {                            \
    if ( sigaction( (signal), &sig_action, NULL ) < 0 ) {		\
      err( EXIT_FAILURE, "sigaction" );                         \
    }                                                           \
  } while ( 0 )


void set_signal_handler( void ) {

  struct sigaction sig_action;

  sig_action.sa_sigaction = posix_signal_handler;
  sig_action.sa_flags     = SA_SIGINFO | SA_RESETHAND;

  sigemptyset( &sig_action.sa_mask );

  HANDLE_SIGNAL( SIGSEGV );
  HANDLE_SIGNAL( SIGFPE  );
  HANDLE_SIGNAL( SIGALRM );
  HANDLE_SIGNAL( SIGINT  );
  HANDLE_SIGNAL( SIGILL  );
  HANDLE_SIGNAL( SIGTERM );
  HANDLE_SIGNAL( SIGABRT );
}

#define RESET_SIGNAL( signal )                                      \
  if ( sigaction( signal, &this.env.orig_##signal, NULL ) < 0 ) {   \
    FATAL( "sigaction: %s\n", strerror( errno ) );                  \
  }

void reset_signal_handler( void ) {
  RESET_SIGNAL( SIGSEGV );
  RESET_SIGNAL( SIGFPE );
  RESET_SIGNAL( SIGALRM );
  RESET_SIGNAL( SIGINT );
  RESET_SIGNAL( SIGILL );
  RESET_SIGNAL( SIGTERM );
  RESET_SIGNAL( SIGABRT );
}

void posix_signal_handler( int sig, siginfo_t *siginfo, void *context ) {

  alarm( 0 );

  if ( SIGABRT != sig || !this.env.expect_abort ) {
    switch ( sig ) {
      SIG_CASE( SIGSEGV, "Segmentation fault" );
      SIG_CASE( SIGINT , "Got Ctrl-C" );
      SIG_CASE( SIGALRM, "Reached time-out" );
      SIG_CASE( SIGTERM, "Received termination request" );
      SIG_CASE( SIGABRT, "Aborted" );
    case SIGFPE:
      switch( siginfo->si_code ) {
        SIG_SUB_CASE( SIGFPE, FPE_INTDIV, "Integer divide by zero" );
        SIG_SUB_CASE( SIGFPE, FPE_INTOVF, "Integer overflow" );
        SIG_SUB_CASE( SIGFPE, FPE_FLTDIV, "Floating point divide by zero" );
        SIG_SUB_CASE( SIGFPE, FPE_FLTOVF, "Floating point overflow" );
        SIG_SUB_CASE( SIGFPE, FPE_FLTUND, "Floating point underflow" );
        SIG_SUB_CASE( SIGFPE, FPE_FLTRES, "Floating point inexact result" );
        SIG_SUB_CASE( SIGFPE, FPE_FLTINV, "Floating point invalid operation" );
        SIG_SUB_CASE( SIGFPE, FPE_FLTSUB, "Subscript out of range" );
      default:
        SIG_CASE_PRINT( "SIGFPE", "Arithmetic exception" );
        break;
      }
      break;
    case SIGILL:
      switch( siginfo->si_code ) {
        SIG_SUB_CASE( SIGILL, ILL_ILLOPC, "Illegal opcode" );
        SIG_SUB_CASE( SIGILL, ILL_ILLOPN, "Illegal operand" );
        SIG_SUB_CASE( SIGILL, ILL_ILLADR, "Illegal addressing mode" );
        SIG_SUB_CASE( SIGILL, ILL_ILLTRP, "Illegal trap" );
        SIG_SUB_CASE( SIGILL, ILL_PRVOPC, "Privileged opcode" );
        SIG_SUB_CASE( SIGILL, ILL_PRVREG, "Privileged register" );
        SIG_SUB_CASE( SIGILL, ILL_COPROC, "Coprocessor error" );
        SIG_SUB_CASE( SIGILL, ILL_BADSTK, "Internal stack error" );
      default:
        SIG_CASE_PRINT( "SIGILL", "Illegal instruction" );
        break;
      }
      break;

    default :
      break;
    }

    fputs( "\n\n", this.stderr_orig );
  }

  if ( SIGINT  == sig ) {
    this.suite.interrupted++;
    this.total.interrupted++;
  }
  if ( SIGABRT == sig ) {
    this.suite.aborted++;
    this.total.aborted++;
  }
  if ( SIGSEGV == sig ) {
    this.suite.segfaulted++;
    this.total.segfaulted++;
  }
  if ( SIGALRM == sig ) {
    this.suite.timed_out++;
    this.total.timed_out++;
  }

  if ( ( SIGSEGV == sig && !this.cont_on_sigsegv ) || SIGTERM == sig ) {
    _exit( EXIT_FAILURE );
  }

  siglongjmp( this.env.restart, 1 );

  (void)context; /* unused */
}

int   test_oracle_start__( char *func, int line, const FILE *fp, char *captured ) {

  if ( this.in_oracle_test ) {
    FATAL( "[%s:%d] DO NOT MIX test_oracle_start()'s inside one another!\n",
           func, line );
  }

  set_signal_handler();

  this.in_oracle_test  = 1;
  this.captured_output = captured;

  fflush( stdout );

  if ( this.orc.captured.unlink ) {
    unlink( this.orc.captured.file );
  }

  this.env.func = func;
  this.env.line = line;

  snprintf( this.orc.captured.file, 1024, "captured-%s-%d.%s", this.env.func, this.env.line, this.captured_output );

  if ( stdout == fp ) {
    stdout = fopen( this.orc.captured.file, "w" );
    if ( NULL == stdout ) {
      fprintf( this.stderr_orig, "Error: Unable to open '%s': %s.\n", this.orc.captured.file, strerror( errno ) );
      return -1;
    }
  }
  else if ( stderr == fp ) {
    stderr = fopen( this.orc.captured.file, "w" );
    if ( NULL == stderr ) {
      fprintf( this.stderr_orig, "Error: Unable to open '%s': %s.\n", this.orc.captured.file, strerror( errno ) );
      return -1;
    }
  }
  else {
    fprintf( this.stderr_orig, "Error: Unknown source (either stdout or stderr).\n" );
    return -1;
  }

  this.total.total++;
  this.suite.total++;

  return 0;
}

static char *load_file_contents( char *filename ) {
  long   len = -1;
  FILE   *fp = fopen( filename, "r" );
  char *contents = NULL;

  if ( NULL == fp ) {
    ERROR( "Unable to open '%s' at %s:%d: %s.\n", filename, this.env.func, this.env.oracle_line, strerror( errno ) );
    return NULL;
  }

  fseek( fp, 0L, SEEK_END );
  len = ftell( fp );
  fseek( fp, 0L, SEEK_SET );

  contents = calloc( 1+len, sizeof( contents[0] ) );
  assert( contents );

  if ( fread( contents, sizeof( contents[0] ), len, fp ) < (size_t)len ) {
    fprintf( this.stderr_orig, "Error: Unable to read from '%s': %s.\n", filename, strerror( errno ) );
    fclose( fp );
    free( contents );
    return NULL;
  }

  fclose( fp );

  return contents;
}

int   test_oracle_compare( char *msg, char *oracle, char *oracle_raw, char *source ) {
  char     *captured = NULL;
  char          *ptr;

  if ( !strcmp( "stdout", source ) ) {
    fclose( stdout );
    stdout = this.stdout_orig;
  }
  else {
    fclose( stderr );
    stderr = this.stderr_orig;
  }

  if ( this.env.verbose ) {
    TEST_STYLE_DESCR();
    fprintf( this.stderr_orig, "%s", msg );
    STYLE_RESET( this.stderr_orig );
    fprintf( this.stderr_orig, ": " );
  }

  captured = load_file_contents( this.orc.captured.file );

  if ( NULL == captured || NULL == oracle_raw ) {
    free( captured );
    free( oracle_raw );
    if ( this.env.verbose ) {
      TEST_STYLE_ERROR();
      fprintf( this.stderr_orig, "FAILED" );
      STYLE_RESET( this.stderr_orig );
    }

    fprintf( this.stderr_orig, "\n" );
    return -1;
  }

  if ( this.env.verbose > 2 ) {
    INFO( "\n{capture-start}\n" );
    fputs( captured, stderr );
    INFO( "\n{capture-end}\n" );
  }


  for ( ptr = oracle + strlen( oracle ) ; ptr > oracle && '\n' == ptr[-1] ; ptr-- ) {
    ptr[-1] = '\0';
  }
  for ( ptr = captured + strlen( captured ) ; ptr > captured && '\n' == ptr[-1] ; ptr-- ) {
    ptr[-1] = '\0';
  }

  if ( strcmp( oracle, captured ) ) {
    size_t exp_len = strlen( oracle );
    size_t    olen = strlen( captured );
    size_t max_len = 0;
    size_t min_len = exp_len < olen ? exp_len : olen;
    for ( max_len = 0; max_len < min_len ; max_len++ ) {
      if ( oracle[ max_len ] != captured[ max_len ] ) break;
    }

    this.orc.captured.unlink = 0;

    if ( this.env.verbose ) {
      size_t i = 0;
      TEST_STYLE_ERROR();
      fprintf( this.stderr_orig, "FAILED" );
      STYLE_RESET( this.stderr_orig );

      fprintf( this.stderr_orig, "\n (Check %s)\n ", this.orc.captured.file );
      TEST_STYLE_EMPH_LIGHT();
      fprintf( this.stderr_orig, "Expected" );
      STYLE_RESET( this.stderr_orig );
      fprintf( this.stderr_orig, " %s", '\0' == *oracle ? "nothing" : "'" );
      for ( i = 0 ; i < max_len ; i++ ) {
        fprintf( this.stderr_orig, "%c", oracle[ i ] );
      }
      TEST_STYLE_ERROR();
      for ( ; i < exp_len ; i++ ) {
        fprintf( this.stderr_orig, "%c", oracle[ i ] );
      }
      STYLE_RESET( this.stderr_orig );
      if ( *oracle ) fprintf( this.stderr_orig, "'" );
      fprintf( this.stderr_orig, "\n " );
      TEST_STYLE_EMPH_LIGHT();
      fprintf( this.stderr_orig, "But got" );
      STYLE_RESET( this.stderr_orig );
      fprintf( this.stderr_orig, "%s", *captured ? ": '" : " nothing" );
      for ( i = 0 ; *captured && i < max_len ; i++ ) {
        fprintf( this.stderr_orig, "%c", captured[ i ] );
      }
      TEST_STYLE_ERROR();
      for ( ; *captured && i < olen ; i++ ) {
        fprintf( this.stderr_orig, "%c", captured[ i ] );
      }
      STYLE_RESET( this.stderr_orig );
      if ( *captured ) fprintf( this.stderr_orig, "'" );
      fprintf( this.stderr_orig, "\n" );
    }

    free( captured );

    return -1;
  }

  this.total.passed++;
  this.suite.passed++;

  if ( this.env.verbose ) {
    TEST_STYLE_SUCCESS();
    fprintf( this.stderr_orig, "PASSED" );
    STYLE_RESET( this.stderr_orig );
    fprintf( this.stderr_orig, ".\n" );
  }

  free( captured );

  return 0;
}

int   test_oracle_check__( char *msg, char *oracle, ... ) {
  size_t  len = 1024 + ( oracle ? strlen( oracle ) : 0 );
  char   *orc = calloc( len+1, sizeof( *orc ) );
  va_list  va;
  int     ret;

  /* WARNING( "CHECK IN ORACLE TEST: %d\n", this.in_oracle_test ); */
  this.in_oracle_test = 0;

  va_start( va, oracle );
  vsnprintf( orc, len, oracle, va );
  va_end( va );

  this.orc.captured.unlink = 1;

  ret = test_oracle_compare( msg, orc, oracle, this.captured_output );
  free( orc );


  return ret;
}

int   test_oracle_check_file__( char *msg, char *oracle_file, ... ) {
  char    file [2048];
  int      ret = -1;
  char *oracle = NULL;
  size_t   len;
  char    *orc;
  va_list   va;

  this.in_oracle_test = 0;

  snprintf( file, sizeof( file ), "%s/%s", this.env.TEST_ORACLES, oracle_file );

  oracle = load_file_contents( file );

  len = 1024 + ( oracle ? strlen( oracle ) : 0 );
  orc = calloc( len+1, sizeof( *orc ) );
  assert( orc );

  va_start( va, oracle_file );
  if ( oracle ) {
    vsnprintf( orc, len, oracle, va );
  }
  va_end( va );

  this.orc.captured.unlink = 1;

  ret = test_oracle_compare( msg, orc, oracle, this.captured_output );

  free( oracle );
  free( orc );

  return ret;
}


static void print_test_usage( int show_easter_eggs, char *progname, char *real_progname ) {
  fprintf( stderr, "\n SYNOPSIS\n\n" );
  fprintf( stderr, "  %s [OPTIONS]\n\n", progname );
  fprintf( stderr, " OPTIONS\n\n" );
  fprintf( stderr, "  -v+ | --verbose[=<level>]\tIncrease test verbosity\n" );
  fprintf( stderr, "  -g  | --debug  \t\tTest is being run in a debugger\n" );
  fprintf( stderr, "  -s  | --summary[=<file>] \tPrint tests summary (and save to file if present)\n" );
  if ( show_easter_eggs )
    fprintf( stderr, "  -c  | --cont-on-sigsegv  \tContinue after segfaults\n" );
  fprintf( stderr, "  -h  | --help   \t\tPrint this help\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "  --unitest-timeout=<timeout>\tStop test after <timeout> secs (dflt: 2)\n" );
  fprintf( stderr, "  --unitest-oracles=<orc_dir>\tPrefix oracle file names with <orc_dir>/\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, " NOTES\n\n" );
  fprintf( stderr, "  1. unitest-timeout and unitest-oracles supersede environment variables\n" );
  fprintf( stderr, "     UNITEST_TIMEOUT and UNITEST_ORACLES, respectively.\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "  2. verbose and debug supersede environment variables\n" );
  fprintf( stderr, "     UNITEST_VERBOSE and UNITEST_GDB, respectively.\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "  3. Integer values may be suffixed with k, K, m or M.\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, " EXAMPLE\n\n" );
  fprintf( stderr, "  Launch test in verbose mode with summary: \n\n");
  fprintf( stderr, "  $ %s -v -s\n", real_progname );
  fprintf( stderr, "\n" );
}

static char *get_runtime_string( char *dflt, char *opt_name, int argc, char *argv[] ) {
  char      *ret = dflt;

  if ( opt_name ) {
    int              i;
    char     *from_env = NULL;
    char      env_name[ 1024 ];
    char opt_real_name[ 1024 ];

    for ( i = 0 ; opt_name[ i ] || ( env_name[ i ] = '\0' ) ; i++ ) {
      switch ( opt_name[ i ] ) {
      case '-' : env_name[ i ] = '_';
        break;
      default  : env_name[ i ] = toupper( opt_name[ i ] );
        break;
      }
    }

    from_env = getenv( env_name );

    if ( from_env ) ret = from_env;

    snprintf( opt_real_name, 1023, "--%s=", opt_name );

    for ( i = 1 ; i < argc ; i++ ) {

      if ( argv[ i ] == strstr( argv[ i ], opt_real_name ) ) {
        return argv[ i ] + strlen( opt_real_name );
      }
    }
  }

  return ret;
}

static long int get_runtime_int( char *dflt, char *opt_name, int argc, char *argv[] ) {
  char      *val = get_runtime_string( dflt, opt_name, argc, argv );
  char      *end = NULL;
  long int   ret = 0;

  errno = 0;
  ret   = strtol( val, &end, 0 );

  if ( ( errno == ERANGE && ( ret == LONG_MAX || ret == LONG_MIN ) )
      || ( errno != 0 && ret == 0 ) ) {
    fprintf( stderr, "Unable to read integer: %s, aborting.\n", strerror( errno ) );
    abort();
  }

  if ( 0 == ret ) ret = 1;

  switch ( *end ) {
  case 'k' : ret *= 1e3;       break;
  case 'K' : ret *= 1024;      break;
  case 'm' : ret *= 1e6;       break;
  case 'M' : ret *= 1024*1024; break;
  default : break;
  }

  return ret;
}

void    unit_test( int argc, char *argv[] ) {
  int          i = 0;
  int  want_help = 0;
  char      *ptr = NULL;
  char  *summary = NULL;

  atexit( test_finish );

  this.stdout_orig     =   stdout;
  this.stderr_orig     =   stderr;
  this.env.debug       = !!getenv( "UNITEST_GDB" );
  this.env.verbose     = !!getenv( "UNITEST_VERBOSE" );
  this.cont_on_sigsegv =   0;
  this.summary.on      =   0;
  this.summary.file    =   NULL;

  sigaction( SIGSEGV, NULL, &this.env.orig_SIGSEGV );
  sigaction( SIGFPE , NULL, &this.env.orig_SIGFPE  );
  sigaction( SIGALRM, NULL, &this.env.orig_SIGALRM );
  sigaction( SIGINT , NULL, &this.env.orig_SIGINT  );
  sigaction( SIGILL , NULL, &this.env.orig_SIGILL  );
  sigaction( SIGTERM, NULL, &this.env.orig_SIGTERM );
  sigaction( SIGABRT, NULL, &this.env.orig_SIGABRT );

  memcpy( this.env.real_prog_name, argv[ 0 ], 1+strlen( argv[ 0 ] ) );

  for ( ptr = strtok( argv[ 0 ], "/" ) ; ptr ; this.env.prog_name = ptr, ptr = strtok( NULL, "/" ) ) {
    if ( argv[ 0 ] != ptr ) ptr[ -1 ] = '/';
  }

  for ( i = 1 ; i < argc ; i++ ) {

    if ( '-' == argv[ i ][ 0 ] ) {

      if ( '-' == argv[ i ][ 1 ] ) {
        if ( !strcmp( argv[ i ], "--help" ) )    want_help++;
        if ( !strcmp( argv[ i ], "--cont-on-sigsegv" ) )    this.cont_on_sigsegv++;
        if ( argv[ i ] == strstr( argv[ i ], "--summary" ) ) {
          if ( '=' == argv[ i ][ 9 ] && strlen( &argv[ i ][ 10 ] ) ) {
            summary = &argv[ i ][ 10 ];
          }
        }
        if ( argv[ i ] == strstr( argv[ i ], "--verbose" ) ) {
          if ( '=' == argv[ i ][ 9 ] ) {
            unsigned level = 0;
           if ( 1 == sscanf( argv[ i ] + 10, "%u", &level ) ) this.env.verbose += level;
          }
          else this.env.verbose++;
        }
        if ( !strcmp( argv[ i ], "--debug" ) )   this.env.debug++;
      }
      else {
        for ( int c = 1 ; argv[ i ][ c ] ; c++ ) {
          if ( argv[ i ][ c ] == 'h' ) want_help++;
          if ( argv[ i ][ c ] == 's' ) {
            this.summary.on = 1;
            if ( '=' == argv[ i ][ c+1 ] && strlen( &argv[ i ][ c+2 ] ) ) {
              summary = &argv[ i ][ c+2 ];
            }
          }
          if ( argv[ i ][ c ] == 'v' ) {
            if ( '=' == argv[ i ][ c+1 ] ) {
              unsigned level = 0;
              if ( 1 == sscanf( argv[ i ] + c+2, "%u", &level ) ) this.env.verbose += level;
            }
            else {
              this.env.verbose++;
            }
          }
          if ( argv[ i ][ c ] == 'g' ) this.env.debug++;
          if ( argv[ i ][ c ] == 'c' ) this.cont_on_sigsegv++;
        }
      }
    }
  }

  if ( !!want_help ) {
    print_test_usage( want_help-1, this.env.prog_name, argv[ 0 ] );
    exit( EXIT_SUCCESS );
  }

  this.env.TEST_ORACLES = get_runtime_string( ".", "test-oracles", argc, argv );
  this.env.TEST_TIMEOUT = get_runtime_int( "2", "test-timeout", argc, argv );

  this.env.debug = !!this.env.debug;

  if ( summary ) {
    this.summary.file = fopen( summary, "at" );
    if ( !this.summary.file ) {
      fprintf( stderr, "Unable to open summary file '%s' for writing: %s.\n", summary, strerror( errno ) );
    }
    /* else { */
    /*   fprintf( this.summary.file, "%-20s\t%20s\t%5s\t%6s\t%10s\t%6s\t%7s\t%10s\n", "Test", "Description", "Count", "Passed", "Segfaults", "Aborts", "Interr.", "Time-outs" ); */
    /* } */
  }
}
