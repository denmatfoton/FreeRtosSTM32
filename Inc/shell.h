/***************************COPYRIGHT INFORMATION*********************************
* Copyright (c)  Link_A_Media Devices, Inc.  All rights reserved.
*
* This file is the Confidential and Proprietary product of
* Link_A_Media Devices, Inc.  Any unauthorized use, reproduction
* or transfer of this file is strictly prohibited.
**********************************************************************************/
/**
* PROJECT:
* DEPARTMENT :     SSD Firmware
* KEYWORDS :
* @version $Revision: #1 $
* @date $Date: 2016/11/21 $
* @auther $Author: andrej.konan $
**********************************************************************************/
#ifndef _SHELL_COMMON_H_
#define _SHELL_COMMON_H_
/***************************COPYRIGHT INFORMATION******************************
 * Copyright (c)  Link_A_Media Devices, Inc.  All rights reserved.            *
 *                                                                            *
 * This file is the Confidential and Proprietary product of                   *
 * Link_A_Media Devices, Inc.  Any unauthorized use, reproduction             *
 * or transfer of this file is strictly prohibited.                           *
 ******************************************************************************/
/** @file           shell.h
* @brief            shell task and its associated data to keep track the command
*                   level and pass the command to the corresponding sublevel command
*

* PROJECT:         SOC-LITE and SOC-LITE2 FW
* MODULE:          shell task and command shell command parsing routine for the debug purpose
* DEPARTMENT :     System Integration
* KEYWORDS :       Shell|Command Parsing|Debug
* @version         $Revision: #1 $
* @date            $Date: 2016/11/21 $
**********************************************************************************/
#include <stdio.h>
#include "main.h"
#include "text_output.h"

/*****************  MACROS *** START ******************************************/
#define Shell_Printf    UART_printf_wait

/*****************  MACROS *** END   ******************************************/

#define MAX_COMMAND_LINE    128                 /// @def max. command line length (in chars)
#define MAX_TOKENS          14                  /// @def max. tokens per command  kpdpbg  changed from 10 to 14
#define MAX_MODULE_NAME     14                  /// @def max. module name length (in chars)
#define MAX_MODULES         5                   /// @def max. number of modules
#define MAX_INT_ARGV        14                  /// @def max. number of integer arguments
#define SHELL_HISTORY_CMDLINE_LEN        24     /// @def length of one commad line history element. Any command longer will be cut.
#define SHELL_HISTORY_MAX_COUNT          10     /// @def max. count of history elements
#define SHELL_HISTORY_INVALID_IDX        0xFF   /// @def invalid index of command history element

// Error Codes
#define ERR_NO_ERROR        0
#define ERR_TRDB_FAIL       1
#define ERR_CMD_FAIL        2
#define ERR_INVALID_PARAM   3
#define ERR_CRC             4
#define ERR_CMD_UNSUPPORTED 5

///@brief command handler prototype
typedef INT32U          (*CMD_PROC)(char *cmdline, int *idx, char *cmd);

///@brief command help handler prototype
typedef void            (*HELP_RPOC)(void);

typedef struct
{
    char cmdLine[SHELL_HISTORY_CMDLINE_LEN];
} Shell_HistoryElement;

/** @struct _Shell
* @brief structure to support multi-level command shell. Each submenu is termed as
*        "module" and linked into the main menu.
*        the shell can also remember last command typed. Use "!" to retrieve last command
*/
typedef struct _Shell
{
    INT16S      mNumTokens;                         ///< Number of parsed tokens in command line
    int         mIndex;                             ///< current token index

    char        mCommandString[MAX_COMMAND_LINE];   ///< user command string
    char        mWorkingString[MAX_COMMAND_LINE];   ///< working copy of command string

    char*       mpToken[MAX_TOKENS];                ///< command token pointer list

    INT16S      pCmd;                               ///< Command String point <    MAX_COMMAND_LINE
    INT16S      mIntArgC;                           ///< number of integer arguments, valid after Shell_IntArgC()
    int         mIntArgV[MAX_INT_ARGV];             ///< integer arguments, valid after Shell_IntArgC()

#if SHELL_COMMAND_HISTORY
    Shell_HistoryElement  cmdHistory[SHELL_HISTORY_MAX_COUNT];      // raw buffer for storage of command lines, one by one
    INT8U       cmdHistoryHead;                     // index of entry to be used on the next HistorySave()
    INT8U       cmdHistoryRecall;                   // index of entry to be restored; SHELL_HISTORY_INVALID_IDX if there is no walking through the cmd history
#endif  // SHELL_COMMAND_HISTORY
} Shell;


/**
 * @brief Shell struct based command handler prototype
 * @param ps    : pointer to shell structure
 * @param idx   : pointer to index of first argument token
 * @param cmd   : command
 */
typedef INT32U          (*SH_CMD)(Shell *ps, int *idx, char* cmd);

/**
 * @struct CmdPairStruct
 * @brief  structure to support table based command dispatching and help printing.
 *
 * @note : null terminated.
 */
typedef __packed struct _CmdPairStruct
{
    char*    mCmdStr;
    char*    mHelpStr;
    SH_CMD   mCmdProc;
} CmdPairStruct;

char _getkey(void);

extern Shell*  pShell;


#ifdef SHELL

#define SHELL_INPUT_CHAR            (0)
#define SHELL_INPUT_EOLINE          (1)
#define SHELL_INPUT_ESCAPE          (2)
#define SHELL_INPUT_ESCAPE_SEQUENCE (3)
int Shell_getALine(char c);

//  Constructor and create Shell task.
INT32U Shell_Create(void);

/// the shell task, created in Shell_Create()
void Shell_task(char c);


/**
* @brief       Scans a string argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*/
unsigned long Shell_ScanStr(char *cmd, int *idx, char *result, int n);

/**
* @brief       Scans a string argument.  Advances the parameter
*              index if successful. Same as Shell_ScanStr, except shell
*              pointer is used.
*/
unsigned long Shell_ScanStrSh(Shell *ps, int *idx, char *result, int n);

/**
* @brief       Scans an integer argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*/
unsigned long Shell_ScanInt(char *cmd, int *idx,  int *num);

/**
* @brief       Scans an integer argument.  Advances the parameter
*              index if successful. Same as Shell_ScanInt, except shell
*              pointer is used.
*/
unsigned long Shell_ScanIntSh(Shell* ps, int *idx,  int *num);

/**
* @brief       Scans a hex argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*/
unsigned long Shell_ScanHex(char *cmd, int *idx, unsigned int *num);

/**
* @brief       Scans a hex argument. Advances the parameter
*              index if successful. Same as Shell_ScanHex, except shell
*              pointer is used.
*/
unsigned long Shell_ScanHexSh(Shell *ps, int *idx, unsigned int *num);

#ifdef SHELL_SCAN_FLOAT
/**
* @brief       Scans a float argument.  Called by a module's
*              command parser (cmd_proc).  Advances the parameter
*              index if successful.
*/
unsigned long Shell_ScanFloat(char *cmd, int *idx, float *num);
#endif


unsigned long Shell_ScanHexLongSh(Shell* ps, int* idx, unsigned int* num);


/**
 *  @brief dispatch a command according to a command table.
 */
BOOL Shell_DispatchCmdTbl(Shell *ps, int* idx, char* cmd, CmdPairStruct *pCmdTbl);

/**
 *  @brief display help according to a command table.
 */
void Shell_HelpCmdTbl(char* cmd, CmdPairStruct *pCmdTbl);

/**
 *  @brief Count and store possible integer numbers from argument tokens.
 */
int  Shell_IntArgC(Shell *ps, int idx);

#else       // SHELL

#define Shell_Create()  (1)

#define Shell_ScanStr(cmd,idx,result,n)     (1)

#define Shell_ScanStrSh(ps,idx,result,n)     (1)

#define Shell_ScanInt(cmd,idx,num)  (1)

#define Shell_ScanIntSh(ps,idx,num)  (1)

#define Shell_ScanHex(cmd,idx,num)  (1)

#define Shell_ScanHexSh(ps,idx,num)  (1)

#ifdef SHELL_SCAN_FLOAT
#define Shell_ScanFloat(cmd,idx,num)  (1)
#endif

#define Shell_DispatchCmdTbl(ps, idx, cmd, pCmdTbl)  (1)

#define Shell_HelpCmdTbl(cmd, pCmdTbl)      (1)

#define Shell_IntArgC(ps, idx)    (0)
#endif    // SHELL


extern void Shell_prompt(void);

void Shell_PerformCommandLine(void);
void Shell_SetCommandLine(char* cmdLine);


#endif  // _SHELL_COMMON_H_
