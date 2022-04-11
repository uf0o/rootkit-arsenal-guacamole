#pragma once

// Use the following to replace argv[0] argv[1] argv[2] 

#define ARGV_EXENAME	argv[0]
#define ARGV_OPERATION	argv[1] 
#define ARGV_OPERAND	argv[2]

#define MAX_CMDLINE_ARGS 3		// argv[0], argv[l], argv[2]
#define MIN_CMDLINE_ARGS 2		// argv[0], argv[l], argv[2]
#define MAX_ARGV_SZ		 127	// size limit for argv[2]

#define MAX_OPERATION_SZ 2		// op-code consist of 2 char


// all commands that can be issues
#define CMD_TEST_OP		"op"			