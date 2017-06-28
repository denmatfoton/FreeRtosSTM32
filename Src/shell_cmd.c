#include <string.h>
#include "shell.h"
#include "cmsis_os.h"
#include "stm32f3xx_hal.h"
#include "lsm303dlhc_driver.h"


INT32U Shell_cmdHelp(Shell *ps, int *idx, char *cmd);
INT32U Shell_showAccel(Shell *ps, int *idx, char *cmd);
INT32U Shell_setAccelParameters(Shell *ps, int *idx, char *cmd);


const CmdPairStruct CmdPairs[] =
{                                                                    
    {"help",            "show available commands",                            Shell_cmdHelp},
    {"accel",           "show accelerometer state",                           Shell_showAccel},
    {"apar",            "set accelerometer params",                           Shell_setAccelParameters},

    {NULL, NULL, NULL}        // terminator
};



/**
 * @brief  process when user input "help" command at shell.
 * @param ps    : pointer to active shell structure
 * @param idx   : pointer to index of first argument token
 * @param cmd   : command
 */
INT32U Shell_cmdHelp(Shell *ps, int *idx, char *cmd)
{
    CmdPairStruct* pCmdTbl = (CmdPairStruct*)CmdPairs;
    
    while (pCmdTbl->mCmdStr != NULL)
    {
        Shell_Printf("%-15s : %s.\n", pCmdTbl->mCmdStr, pCmdTbl->mHelpStr);

        pCmdTbl++;
    }
    
    return ERR_NO_ERROR;
}


extern osSemaphoreId  compasSemaphore;

INT32U Shell_showAccel(Shell *ps, int *idx, char *cmd)
{
    osSemaphoreRelease(compasSemaphore);
    
    return ERR_NO_ERROR;
}


INT32U Shell_setAccelParameters(Shell *ps, int *idx, char *cmd)
{
    int param = ps->mIntArgV[0];
    int value = ps->mIntArgV[1];
        
    switch(param)
    {
        case 0:
            SetHPFMode(value);
            break;
        case 1:
            SetHPFCutOFF(value);
            break;
        case 2:
            SetFilterDataSel(value);
            break;
        default:
            break;
    }
    
    return ERR_NO_ERROR;
}


