
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "circuit.h"
#include "ttyio.h"
#include "sparse/spmatrix.h"


// Perform the convergence test - returns 1 if any of the 
// values in the old and cur arrays have changed by more 
// than absTol + relTol*(max(old,cur)), otherwise returns 0.
//
int
sCKT::NIconvTest()
{
    int size = CKTmatrix->spGetSize(1);
    for (int i = 1; i <= size; i++) {

        sCKTnode *node = CKTnodeTab.find(i);
        if (!node)
            // shouldn't happen
            break;

        double cur = *(CKTrhs + i);
        double old = *(CKTrhsOld + i);
        double acur = fabs(cur);
        double aold = fabs(old);
        double dd = fabs(cur - old);
        double rel = CKTcurTask->TSKreltol*SPMAX(aold, acur);

        if (node->type() == SP_VOLTAGE) {
            double tol = rel + CKTcurTask->TSKvoltTol;
            if (dd > tol) {
                if (CKTstepDebug)
                    TTY.err_printf(" non-convergence at v-node %s, %g %g\n",
                        (char*)nodeName(i), old, cur);
                CKTtroubleNode = i;
                return (1);
            }
        }
        else {
            double tol = rel + CKTcurTask->TSKabstol;
            if (dd > tol) {
                if (CKTstepDebug)
                    TTY.err_printf(" non-convergence at i-node %s, %g %g\n",
                        (char*)nodeName(i), old, cur);
                return (1);
            }
        }
    }
    if (convTest() != OK)
        CKTnoncon++;
    int ret = (CKTnoncon != 0);
    if (CKTstepDebug && ret)
        TTY.err_printf(" non-convergence from convTest()\n");
     return (ret);
}

