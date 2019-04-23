
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "tjmdefs.h"


int
TJMdev::accept(sCKT *ckt, sGENmodel *genmod)
{
    sTJMmodel *model = static_cast<sTJMmodel*>(genmod);
    for ( ; model; model = model->next()) {

        bool didm = false;
        double vmax = 0;

        sTJMinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            // keep phase  >= 0 and < 2*PI
            double phi = *(ckt->CKTstate0 + inst->TJMphase);
            int pint = *(int *)(ckt->CKTstate1 + inst->TJMphsInt);
/*
            if (phi >= 2*M_PI) {
                phi -= 2*M_PI;
                pint++;
            }
            else if (phi < 0) {
                phi += 2*M_PI;
                pint--;
            }
*/
            if (phi >= 4*M_PI) {
                phi -= 4*M_PI;
                pint++;
            }
            else if (phi < 0) {
                phi += 4*M_PI;
                pint--;
            }

            *(ckt->CKTstate0 + inst->TJMphase) = phi;
            *(int *)(ckt->CKTstate0 + inst->TJMphsInt) = pint;

            // find max vj for time step
            if (model->TJMictype != 0 && inst->TJMcriti > 0) {
                if (!didm) {
                    didm = true;
                    if (vmax < model->TJMvdpbak)
                        vmax = model->TJMvdpbak;
                }
                double vj = *(ckt->CKTstate0 + inst->TJMvoltage);
                if (vj < 0)
                    vj = -vj;
                if (vmax < vj)
                    vmax = vj;
            }

            if (inst->TJMphsNode > 0)
                *(ckt->CKTrhsOld + inst->TJMphsNode) = phi + (2*M_PI)*pint;

            inst->tjm_accept(phi);
        }
        if (vmax > 0.0) {
            // Limit next time step.
            double delmax = M_PI*model->TJMtsfact*PHI0_2PI/vmax;
            if (ckt->CKTdevMaxDelta == 0.0 || delmax < ckt->CKTdevMaxDelta)
                ckt->CKTdevMaxDelta = delmax;
        }
    }
    return (OK);
}
