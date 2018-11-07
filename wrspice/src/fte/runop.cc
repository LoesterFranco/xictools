
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "parser.h"
#include "runop.h"
#include "measure.h"
#include "graph.h"
#include "input.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "toolbar.h"
#include "output.h"
#include "rundesc.h"
#include "device.h"
#include "spnumber/spnumber.h"


//
// The stop command and related, general support for runops.
//

// keywords
const char *kw_stop   = "stop";
const char *kw_after  = "after";
const char *kw_at     = "at";
const char *kw_before = "before";
const char *kw_when   = "when";
const char *kw_active = "active";
const char *kw_inactive = "inactive";
const char *kw_call   = "call";


// Step a number of output increments.
//
void
CommandTab::com_step(wordlist *wl)
{
    int n;
    if (wl)
        n = atoi(wl->wl_word);
    else
        n = 1;
    OP.runops()->set_step_count(n);
    OP.runops()->set_num_steps(n);
    Sp.Simulate(SIMresume, 0);
}


// Set up a stop runop, many possibilities see below.
//
void
CommandTab::com_stop(wordlist *wl)
{
    OP.stopCmd(wl);
}


// Print out the currently active breakpoints and traces.
//
void
CommandTab::com_status(wordlist*)
{
    OP.statusCmd(0);
}


// Delete runops, many possibilities see below.
//
void
CommandTab::com_delete(wordlist *wl)
{
    OP.deleteCmd(wl);
}
// End of CommandTab functions.


namespace {
    // Return true if s is a bare integer, perhaps with trailing white
    // space.
    //
    bool is_uint(const char *s)
    {
        if (!s || !isdigit(*s))
            return (false);
        while (*++s) {
            if (isspace(*s))
                break;
            if (!isdigit(*s))
                return (false);
        }
        return (true);
    }

    bool is_kw(const char *s)
    {
        return (lstring::cieq(s, kw_after) || lstring::cieq(s, kw_at) ||
            lstring::cieq(s, kw_before) || lstring::cieq(s, kw_when) ||
            lstring::cieq(s, kw_call));
    }
}


// Set a breakpoint. Possible commands are:
//  stop after|at|before n
//  stop when expr
//
// If more than one is given on a command line, then this is a conjunction.
//
void
IFoutput::stopCmd(wordlist *wl)
{
    sRunop *d = 0, *thisone = 0;
    while (wl) {
        if (thisone == 0)
            thisone = d = new sRunop;
        else {
            d->set_also(new sRunop);
            d = d->also();
        }

        // Figure out what the first condition is.
        if (lstring::cieq(wl->wl_word, kw_at)) {
            d->set_type(RO_STOPAT);
            wl = wl->wl_next;
            bool pt = false;
            if (wl && lstring::ciprefix("p", wl->wl_word)) {
                pt = true;
                wl = wl->wl_next;
            }
            if (pt) {
                int i = 0;
                for (wordlist *w = wl; w; w = w->wl_next) {
                    if (!is_uint(w->wl_word))
                        break;
                    i++;
                }
                if (i) {
                    int *ary = new int[i];
                    i = 0;
                    for ( ; wl; wl = wl->wl_next) {
                        if (!is_uint(wl->wl_word))
                            break;
                        ary[i++] = atoi(wl->wl_word);
                    }
                    d->set_points(i, ary);
                    continue;
                }
            }
            else {
                int i = 0;
                for (wordlist *w = wl; w; w = w->wl_next) {
                    const char *word = w->wl_word;
                    int err;
                    IP.getFloat(&word, &err, true);
                    if (err != OK)
                        break;
                    i++;
                }
                if (i) {
                    double *ary = new double[i];
                    i = 0;
                    for ( ; wl; wl = wl->wl_next) {
                        const char *word = wl->wl_word;
                        int err;
                        double tmp = IP.getFloat(&word, &err, true);
                        if (err != OK)
                            break;
                        ary[i++] = tmp;
                    }
                    d->set_points(i, ary);
                    continue;
                }
            }
            GRpkgIf()->ErrPrintf(ET_ERROR, "stop at: no targets found.\n");
            sRunop::destroy(thisone);
            return;
        }
        if (lstring::eq(wl->wl_word, kw_after) ||
                lstring::eq(wl->wl_word, kw_before)) {
            if (lstring::eq(wl->wl_word, kw_after))
                d->set_type(RO_STOPAFTER);
            else
                d->set_type(RO_STOPBEFORE);
            wl = wl->wl_next;
            bool pt = false;
            if (wl && lstring::ciprefix("p", wl->wl_word)) {
                pt = true;
                wl = wl->wl_next;
            }
            if (wl) {
                const char *word = wl->wl_word;
                if (pt) {
                    if (is_uint(word)) {
                        d->set_point(atoi(word));
                        wl = wl->wl_next;
                        continue;
                    }
                }
                else {
                    int err;
                    double tmp = IP.getFloat(&word, &err, true);
                    if (err == OK) {
                        d->set_point(tmp);
                        wl = wl->wl_next;
                        continue;
                    }
                }
            }
            GRpkgIf()->ErrPrintf(ET_ERROR, "stop %s: no target found.\n",
               d->type() == RO_STOPBEFORE ? "before" : "after");
            sRunop::destroy(thisone);
            return;
        }
        if (lstring::eq(wl->wl_word, kw_when)) {
            d->set_type(RO_STOPWHEN);
            wl = wl->wl_next;
            char *string = 0;
            if (wl) {
                wordlist *wx;
                int i;
                for (i = 0, wx = wl; wx; wx = wx->wl_next) {
                    if (is_kw(wx->wl_word))
                        break;
                    i += strlen(wx->wl_word) + 1;
                }
                i++;
                string = new char[i];
                char *s = lstring::stpcpy(string, wl->wl_word);
                for (wl = wl->wl_next; wl; wl = wl->wl_next) {
                    if (is_kw(wl->wl_word))
                        break;
                    char *t = wl->wl_word;
                    *s++ = ' ';
                    while (*t)
                        *s++ = *t++;
                }
                *s = '\0';
                CP.Unquote(string);
                if (!*string) {
                    delete [] string;
                    string = 0;
                }
            }
            if (string) {
                d->set_string(string);
                continue;
            }
            GRpkgIf()->ErrPrintf(ET_ERROR, "stop when: no expression found.\n");
            sRunop::destroy(thisone);
            return;
        }
        if (lstring::eq(wl->wl_word, kw_call)) {
            wl = wl->wl_next;
            const char *word = wl ? wl->wl_word : 0;
            thisone->set_call(true, word);
            if (wl)
                wl = wl->wl_next;
            continue;
        }
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "stop: unknown keyword following \"stop\".\n");
        sRunop::destroy(thisone);
        return;
    }
    if (thisone) {
        sFtCirc *curcir = Sp.CurCircuit();
        thisone->set_number(o_runops->new_count());
        thisone->set_active(true);
        if (CP.GetFlag(CP_INTERACTIVE) || !curcir) {
            if (o_runops->stops()) {
                for (d = o_runops->stops(); d->next(); d = d->next())
                    ;
                d->set_next(thisone);
            }
            else
                o_runops->set_stops(thisone);
        }
        else {
            if (curcir->stops()) {
                for (d = curcir->stops(); d->next(); d = d->next())
                    ;
                d->set_next(thisone);
            }
            else
                curcir->set_stops(thisone);
        }
    }
    ToolBar()->UpdateTrace();
}


void
IFoutput::statusCmd(char **ps)
{
    if (ps)
        *ps = 0;
    const char *msg = "No runops are in effect.\n";
    if (!o_runops->isset() &&
            (!Sp.CurCircuit() || !Sp.CurCircuit()->runops().isset())) {
        if (!ps)
            TTY.send(msg);
        else
            *ps = lstring::copy(msg);
        return;
    }
    char **t = ps;
    sFtCirc *curcir = Sp.CurCircuit();
    for (sRunop *d = o_runops->stops(); d; d = d->next())
        d->print(t);
    if (curcir) {
        for (sRunop *d = curcir->stops(); d; d = d->next())
            d->print(t);
    }
    for (sRunop *d = o_runops->traces(); d; d = d->next())
        d->print(t);
    if (curcir) {
        for (sRunop *d = curcir->traces(); d; d = d->next())
            d->print(t);
    }
    for (sRunop *d = o_runops->iplots(); d; d = d->next())
        d->print(t);
    if (curcir) {
        for (sRunop *d = curcir->iplots(); d; d = d->next())
            d->print(t);
    }
    for (sRunop *d = o_runops->saves(); d; d = d->next())
        d->print(t);
    if (curcir) {
        for (sRunop *d = curcir->saves(); d; d = d->next())
            d->print(t);
    }
}


// Delete breakpoints and traces. Usage is:
//   delete [[in]active][all | stop | trace | iplot | save | number] ...
// With inactive true (args to right of keyword), the runop is
// cleared only if it is inactive.
//
void
IFoutput::deleteCmd(wordlist *wl)
{
    if (!wl) {
        sRunop *d = o_runops->stops();
        // find the earlist runop
        if (!d ||
            (o_runops->traces() && d->number() > o_runops->traces()->number()))
            d = o_runops->traces();
        if (!d ||
            (o_runops->iplots() && d->number() > o_runops->iplots()->number()))
            d = o_runops->iplots();
        if (!d ||
            (o_runops->saves() && d->number() > o_runops->saves()->number()))
            d = o_runops->saves();
        if (Sp.CurCircuit()) {
            sRunopDb &db = Sp.CurCircuit()->runops();
            if (!d || (db.stops() && d->number() > db.stops()->number()))
                d = db.stops();
            if (!d || (db.traces() && d->number() > db.traces()->number()))
                d = db.traces();
            if (!d || (db.iplots() && d->number() > db.iplots()->number()))
                d = db.iplots();
            if (!d || (db.saves() && d->number() > db.saves()->number()))
                d = db.saves();
        }
        if (!d) {
            TTY.printf("No runops are in effect.\n");
            return;
        }
        d->print(0);
        if (CP.GetFlag(CP_NOTTYIO))
            return;
        if (!TTY.prompt_for_yn(true, "delete [y]? "))
            return;
        if (d == o_runops->stops())
            o_runops->set_stops(d->next());
        else if (d == o_runops->traces())
            o_runops->set_traces(d->next());
        else if (d == o_runops->iplots())
            o_runops->set_iplots(d->next());
        else if (d == o_runops->saves())
            o_runops->set_saves(d->next());
        else if (Sp.CurCircuit()) {
            sRunopDb &db = Sp.CurCircuit()->runops();
            if (d == db.stops())
                db.set_stops(d->next());
            else if (d == db.traces())
                db.set_traces(d->next());
            else if (d == db.iplots())
                db.set_iplots(d->next());
            else if (d == db.saves())
                db.set_saves(d->next());
        }
        ToolBar()->UpdateTrace();
        sRunop::destroy(d);
        return;
    }
    bool inactive = false;
    for (; wl; wl = wl->wl_next) {
        if (lstring::eq(wl->wl_word, kw_inactive)) {
            inactive = true;
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_active)) {
            inactive = false;
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_all)) {
            deleteRunop(DF_ALL, inactive, -1);
            return;
        }
        if (lstring::eq(wl->wl_word, kw_stop)) {
            deleteRunop(DF_STOP, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_trace)) {
            deleteRunop(DF_TRACE, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_iplot)) {
            deleteRunop(DF_IPLOT, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_save)) {
            deleteRunop(DF_SAVE, inactive, -1);
            continue;
        }

        char *s;
        // look for range n1-n2
        if ((s = strchr(wl->wl_word, '-')) != 0) {
            if (s == wl->wl_word || !*(s+1)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "badly formed range %s.\n",
                    wl->wl_word);
                continue;
            }
            char *t;
            for (t = wl->wl_word; *t; t++) {
                if (!isdigit(*t) && *t != '-') {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "badly formed range %s.\n",
                        wl->wl_word);
                    break;
                }
            }
            if (*t)
                continue;
            int n1 = atoi(wl->wl_word);
            s++;
            int n2 = atoi(s);
            if (n1 > n2) {
                int nt = n1;
                n1 = n2;
                n2 = nt;
            }
            while (n1 <= n2) {
                deleteRunop(DF_ALL, inactive, n1);
                n1++;
            }
            continue;
        }
                
        for (s = wl->wl_word; *s; s++) {
            if (!isdigit(*s)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s isn't a runops number.\n",
                    wl->wl_word);
                break;
            }
        }
        if (*s)
            continue;
        int i = atoi(wl->wl_word);
        deleteRunop(DF_ALL, inactive, i);
    }
    ToolBar()->UpdateTrace();
}


void
IFoutput::initRunops(sRunDesc *run)
{
    if (!run)
        return;
    sRunopDb *db = run->circuit() ? &run->circuit()->runops() : 0;
    // called at beginning of run
    if (o_runops->step_count() != o_runops->num_steps()) {
        // left over from last run
        o_runops->set_step_count(0);
        o_runops->set_num_steps(0);
    }
    for (sRunop *d = o_runops->stops(); d; d = d->next()) {
        for (sRunop *dt = d; dt; dt = dt->also()) {
            if (dt->type() == RO_STOPWHEN)
                dt->set_point(0);
        }
    }
    for (sRunop *dt = o_runops->traces(); dt; dt = dt->next())
        dt->set_point(0);
    for (sRunop *dt = o_runops->iplots(); dt; dt = dt->next()) {
        if (dt->type() == RO_DEADIPLOT) {
            // user killed the window
            if (dt->graphid())
                GP.DestroyGraph(dt->graphid());
            dt->set_type(RO_IPLOT);
        }
        if (run->check() && run->check()->out_mode == OutcCheckSeg)
            dt->set_reuseid(dt->graphid());
        if (!(run->check() && (run->check()->out_mode == OutcCheckMulti ||
                run->check()->out_mode == OutcLoop))) {
            dt->set_point(0);
            dt->set_graphid(0);
        }
    }
    for (sMeas *m = o_runops->measures(); m; m = m->next) {
        if (run->job()->JOBtype != m->analysis)
            continue;
        m->reset(run->runPlot());
    }
    if (db) {
        for (sRunop *d = db->stops(); d; d = d->next()) {
            for (sRunop *dt = d; dt; dt = dt->also()) {
                if (dt->type() == RO_STOPWHEN)
                    dt->set_point(0);
            }
        }
        for (sRunop *dt = db->traces(); dt; dt = dt->next())
            dt->set_point(0);
        for (sRunop *dt = db->iplots(); dt; dt = dt->next()) {
            if (dt->type() == RO_DEADIPLOT) {
                // user killed the window
                if (dt->graphid())
                    GP.DestroyGraph(dt->graphid());
                dt->set_type(RO_IPLOT);
            }
            if (run->check() && run->check()->out_mode == OutcCheckSeg)
                dt->set_reuseid(dt->graphid());
            if (!(run->check() && (run->check()->out_mode == OutcCheckMulti ||
                    run->check()->out_mode == OutcLoop))) {
                dt->set_point(0);
                dt->set_graphid(0);
            }
        }
        for (sMeas *m = db->measures(); m; m = m->next) {
            if (run->job()->JOBtype != m->analysis)
                continue;
            m->reset(run->runPlot());
        }
    }
}


void
IFoutput::setRunopActive(int dbnum, bool state)
{
    for (sRunop *d = o_runops->stops(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (sRunop *d = o_runops->traces(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (sRunop *d = o_runops->iplots(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (sRunop *d = o_runops->saves(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    if (Sp.CurCircuit()) {
        sRunopDb &db = Sp.CurCircuit()->runops();
        for (sRunop *d = db.stops(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (sRunop *d = db.traces(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (sRunop *d = db.iplots(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (sRunop *d = db.saves(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
    }
}


void
IFoutput::deleteRunop(int which, bool inactive, int num)
{
    if (which & DF_SAVE) {
        sRunop *dlast = 0, *dnext;
        for (sRunop *d = o_runops->stops(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_stops(dnext);
                else
                    dlast->set_next(dnext);
                sRunop::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit()) {
            sRunopDb &db = Sp.CurCircuit()->runops();
            dlast = 0;
            for (sRunop *d = db.stops(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db.set_stops(dnext);
                    else
                        dlast->set_next(dnext);
                    sRunop::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (which & DF_TRACE) {
        sRunop *dlast = 0, *dnext;
        for (sRunop *d = o_runops->traces(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_traces(dnext);
                else
                    dlast->set_next(dnext);
                sRunop::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit()) {
            sRunopDb &db = Sp.CurCircuit()->runops();
            dlast = 0;
            for (sRunop *d = db.traces(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db.set_traces(dnext);
                    else
                        dlast->set_next(dnext);
                    sRunop::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (which & DF_IPLOT) {
        sRunop *dlast = 0, *dnext;
        for (sRunop *d = o_runops->iplots(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_iplots(dnext);
                else
                    dlast->set_next(dnext);
                sRunop::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit()) {
            sRunopDb &db = Sp.CurCircuit()->runops();
            dlast = 0;
            for (sRunop *d = db.iplots(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db.set_iplots(dnext);
                    else
                        dlast->set_next(dnext);
                    sRunop::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (which & DF_SAVE) {
        sRunop *dlast = 0, *dnext;
        for (sRunop *d = o_runops->saves(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_saves(dnext);
                else
                    dlast->set_next(dnext);
                sRunop::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit()) {
            sRunopDb &db = Sp.CurCircuit()->runops();
            dlast = 0;
            for (sRunop *d = db.saves(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db.set_saves(dnext);
                    else
                        dlast->set_next(dnext);
                    sRunop::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
}


// Run runops, measures, and margin analysis tests.
//
void
IFoutput::checkRunops(sRunDesc *run, double ref)
{
    if (!run)
        return;
    sCHECKprms *chk = run->check();
    if (chk && chk->out_mode == OutcCheck) {
        if (!chk->points() || ref < chk->points()[chk->index()]) {
            vecGc();
            return;
        }

        chk->evaluate();

        chk->set_index(chk->index() + 1);
        if (chk->failed() || chk->index() == chk->max_index())
            o_endit = true;
        vecGc();
        return;
    }

    if (run->pointCount() > 0) {
        sRunopDb *db = run->circuit() ? &run->circuit()->runops() : 0;
        if (o_runops->measures() || (db && db->measures())) {
            run->segmentizeVecs();
            bool done = true;
            bool stop = false;
            for (sMeas *m = o_runops->measures(); m; m = m->next) {
                if (run->anType() != m->analysis)
                    continue;
                if (!m->check(run->circuit())) {
                    done = false;
                    continue;
                }
                if (m->shouldstop())
                    stop = true;
            }
            if (db) {
                for (sMeas *m = db->measures(); m; m = m->next) {
                    if (run->anType() != m->analysis)
                        continue;
                    if (!m->check(run->circuit())) {
                        done = false;
                        continue;
                    }
                    if (m->shouldstop())
                        stop = true;
                }
            }
            stop &= done;
            if (stop) {
                // Reset stop flags so analysis can be continued.
                for (sMeas *m = o_runops->measures(); m; m = m->next)
                    m->nostop();
                if (db) {
                    for (sMeas *m = db->measures(); m; m = m->next)
                        m->nostop();
                }
            }
            run->unsegmentizeVecs();
            if (stop)
                o_shouldstop = true;
        }
        if (o_runops->traces() || o_runops->stops() ||
                (db && (db->traces() || db->stops()))) {
            bool tflag = true;
            run->scalarizeVecs();
            for (sRunop *d = o_runops->traces(); d; d = d->next())
                d->print_trace(run->runPlot(), &tflag, run->pointCount());
            if (db) {
                for (sRunop *d = db->traces(); d; d = d->next())
                    d->print_trace(run->runPlot(), &tflag, run->pointCount());
            }
            for (sRunop *d = o_runops->stops(); d; d = d->next()) {
                ROret r = d->should_stop(run);
                if (r == RO_PAUSE) {
                    bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                    TTY.printf("%-2d: condition met: stop ", d->number());
                    d->printcond(0);
                    o_shouldstop = true;
                    if (need_pr)
                        CP.Prompt();
                }
                else if (r == RO_ENDIT) {
//XXX handle this
                    o_endit = true;
                }
            }
            if (db) {
                for (sRunop *d = db->stops(); d; d = d->next()) {
                    ROret r = d->should_stop(run);
                    if (r == RO_PAUSE) {
                        bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                        TTY.printf("%-2d: condition met: stop ", d->number());
                        d->printcond(0);
                        o_shouldstop = true;
                        if (need_pr)
                            CP.Prompt();
                    }
                    else if (r == RO_ENDIT) {
//XXX handle this
                        o_endit = true;
                    }
                }
            }
            run->unscalarizeVecs();
        }
        if (o_runops->iplots() || (db && db->iplots())) {
            sDataVec *xs = run->runPlot()->scale();
            if (xs) {
                int len = xs->length();
                if (len >= IPOINTMIN || (xs->flags() & VF_ROLLOVER)) {
                    bool doneone = false;
                    for (sRunop *d = o_runops->iplots(); d; d = d->next()) {
                        if (d->type() == RO_IPLOT) {
                            iplot(d, run);
                            if (GRpkgIf()->CurDev() &&
                                    GRpkgIf()->CurDev()->devtype ==
                                    GRfullScreen) {
                                doneone = true;
                                break;
                            }
                        }
                    }
                    if (db && !doneone) {
                        for (sRunop *d = db->iplots(); d; d = d->next()) {
                            if (d->type() == RO_IPLOT) {
                                iplot(d, run);
                                if (GRpkgIf()->CurDev() &&
                                        GRpkgIf()->CurDev()->devtype ==
                                        GRfullScreen)
                                    break;
                            }
                        }
                    }
                }
            }
        }
        if (o_runops->step_count() > 0 && o_runops->dec_step_count() == 0) {
            if (o_runops->num_steps() > 1) {
                bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                TTY.printf("Stopped after %d steps.\n", o_runops->num_steps());
                if (need_pr)
                    CP.Prompt();
            }
            o_shouldstop = true;
        }
    }

    if (chk && (chk->out_mode == OutcCheckSeg ||
            chk->out_mode == OutcCheckMulti)) {
        if (!chk->points() || ref < chk->points()[chk->index()] ||
                chk->index() >= chk->max_index()) {
            vecGc();
            return;
        }

        if (!chk->failed()) {
            run->scalarizeVecs();
            chk->evaluate();
            run->unscalarizeVecs();
        }

        chk->set_index(chk->index() + 1);
        if ((chk->failed() || chk->index() == chk->max_index()) &&
                chk->out_mode != OutcCheckMulti)
            o_endit = true;
    }

    vecGc();
}


// Check for requested pause.
//
int
IFoutput::pauseTest(sRunDesc *run)
{
    if (!Sp.GetFlag(FT_BATCHMODE))
        GP.Checkup();
    if (Sp.GetFlag(FT_INTERRUPT)) {
        o_shouldstop = false;
        Sp.SetFlag(FT_INTERRUPT, false);
        ToolBar()->UpdatePlots(0);
        endIplot(run);
        return (E_INTRPT);
    }
    else if (o_shouldstop) {
        o_shouldstop = false;
        ToolBar()->UpdatePlots(0);
        endIplot(run);
        return (E_PAUSE);
    }
    else
        return (OK);
}
// End of IFoutput functions.


// Static function.
void
sRunop::destroy(sRunop *d)
{
    while (d) {
        sRunop *x = d;
        d = d->ro_also;
        if (x->ro_type == RO_DEADIPLOT && x->ro_graphid) {
            // User killed the window.
            GP.DestroyGraph(x->ro_graphid);
        }
        delete x;
    }
}


namespace { const char *Msg = "can't evaluate %s.\n"; }

bool
sRunop::istrue()
{
    if (ro_bad)
        return (true);

    wordlist *wl = CP.LexStringSub(ro_string);
    if (!wl) {
        GRpkgIf()->ErrPrintf(ET_ERROR, Msg, ro_string);
        ro_bad = true;
        return (true);
    }
    char *str = wordlist::flatten(wl);
    wordlist::destroy(wl);

    const char *t = str;
    pnode *p = Sp.GetPnode(&t, true);
    delete [] str;
    sDataVec *v = 0;
    if (p) {
        v = Sp.Evaluate(p);
        delete p;
    }
    if (!v) {
        GRpkgIf()->ErrPrintf(ET_ERROR, Msg, ro_string);
        ro_bad = true;
        return (true);
    }
    if (v->realval(0) != 0.0 || v->imagval(0) != 0.0)
        return (true);
    return (false);
}


ROret
sRunop::should_stop(sRunDesc *run)
{
    if (!ro_active || !run)
        return (RO_OK);
    bool when = true;
    bool after = true;
    for (sRunop *dt = this; dt; dt = dt->ro_also) {
        if (dt->ro_type == RO_STOPAFTER) {
            if (dt->ro_ptmode) {
                if (run->pointCount() < dt->ro_p.ipoint) {
                    after = false;
                    break;
                }
            }
            else {
                if (run->ref_value() < dt->ro_p.dpoint) {
                    after = false;
                    break;
                }
            }
        }
        else if (dt->ro_type == RO_STOPAT) {
            after = false;
            if (dt->ro_ptmode) {
                if (dt->ro_index < dt->ro_numpts &&
                        run->pointCount() >= dt->ro_a.ipoints[dt->ro_index]) {
                    dt->ro_index++;
                    after = true;
                    break;
                }
            }
            else {
                if (dt->ro_index < dt->ro_numpts &&
                        run->ref_value() >= dt->ro_a.dpoints[dt->ro_index]) {
                    dt->ro_index++;
                    after = true;
                    break;
                }
            }
        }
        else if (dt->ro_type == RO_STOPBEFORE) {
            if (dt->ro_ptmode) {
                if (run->pointCount() > dt->ro_p.ipoint) {
                    after = false;
                    break;
                }
            }
            else {
                if (run->ref_value() > dt->ro_p.dpoint) {
                    after = false;
                    break;
                }
            }
        }
        else if (dt->ro_type == RO_STOPWHEN) {
            if (!dt->istrue()) {
                when = false;
                break;
            }
        }
    }
    if (when && after) {
        // Call the callback, if any.  This can override the stop.
        if (ro_call) {
            if (ro_callfn) {
                // Call the named script or codeblock.
                wordlist wl;
                wl.wl_word = ro_callfn;
                Sp.ExecCmds(&wl);
                if (CP.ReturnVal() == CB_OK)
                    return (RO_OK);
                if (CP.ReturnVal() == CB_ENDIT)
                    return (RO_ENDIT);
            }
            else if (run->check()) {
                // Run the "controls" bound codeblock.  We stop
                // only if the checkFAIL vector is not
                // set.
//XXX don't use checkFAIL here

                run->check()->evaluate();
                if (!run->check()->failed())
                    return (RO_OK);
            }
            else {
                Sp.CurCircuit()->controlBlk().exec(true);
                if (CP.ReturnVal() == CB_OK)
                    return (RO_OK);
                if (CP.ReturnVal() == CB_ENDIT)
                    return (RO_ENDIT);
            }
        }
        return (RO_PAUSE);
    }
    return (RO_OK);
}


void
sRunop::print(char **retstr)
{
    char buf[BSIZE_SP];
    sRunop *dc = 0;
    const char *msg0 = "%c %-4d %s %s\n";
    const char *msg1 = "%c %-4d stop";
    const char *msg2 = "%c %-4d %s %s";

    switch (ro_type) {
    case RO_SAVE:
        if (!retstr)
            TTY.printf(msg0, ro_active ? ' ' : 'I', ro_number,
                kw_save,  ro_string);
        else {
            sprintf(buf, msg0, ro_active ? ' ' : 'I', ro_number,
                kw_save,  ro_string);
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    case RO_TRACE:
        if (!retstr)
            TTY.printf(msg0, ro_active ? ' ' : 'I', ro_number,
                kw_trace,  ro_string ? ro_string : "");
        else {
            sprintf(buf, msg0, ro_active ? ' ' : 'I', ro_number,
                kw_trace,  ro_string ? ro_string : "");
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    case RO_STOPWHEN:
    case RO_STOPAFTER:
    case RO_STOPAT:
    case RO_STOPBEFORE:
        if (!retstr) {
            TTY.printf(msg1, ro_active ? ' ' : 'I', ro_number);
            printcond(0);
        }
        else {
            sprintf(buf, msg1, ro_active ? ' ' : 'I', ro_number);
            *retstr = lstring::build_str(*retstr, buf);
            printcond(retstr);
        }
        break;

    case RO_IPLOT:
    case RO_DEADIPLOT:
        if (!retstr) {
            TTY.printf(msg2, ro_active ? ' ' : 'I', ro_number,
                kw_iplot, ro_string);
            for (dc = ro_also; dc; dc = dc->ro_also)
                TTY.printf(" %s", dc->ro_string);
            TTY.send("\n");
        }
        else {
            sprintf(buf, msg2, ro_active ? ' ' : 'I', ro_number,
                kw_iplot, ro_string);
            for (dc = ro_also; dc; dc = dc->ro_also)
                sprintf(buf + strlen(buf), " %s", dc->ro_string);
            strcat(buf, "\n");
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    default:
        GRpkgIf()->ErrPrintf(ET_INTERR, "com_status: bad db %d.\n", ro_type);
        break;
    }
}


// Print the node voltages for trace.
//
bool
sRunop::print_trace(sPlot *plot, bool *flag, int pnt)
{
    if (!ro_active)
        return (true);
    if (*flag) {
        sDataVec *v1 = plot->scale();
        if (v1)
            TTY.printf_force("%-5d %s = %s\n", pnt, v1->name(), 
                SPnum.printnum(v1->realval(0), v1->units(), false));
        *flag = false;
    }

    if (ro_string) {
        if (ro_bad)
            return (false);
        wordlist *wl = CP.LexStringSub(ro_string);
        if (!wl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, Msg, ro_string);
            ro_bad = true;
            return (false);
        }

        sDvList *dvl = 0;
        pnlist *pl = Sp.GetPtree(wl, true);
        wordlist::destroy(wl);
        if (pl)
            dvl = Sp.DvList(pl);
        if (!dvl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, Msg, ro_string);
            ro_bad = true;
            return (false);
        }
        for (sDvList *dv = dvl; dv; dv = dv->dl_next) {
            sDataVec *v1 = dv->dl_dvec;
            if (v1 == plot->scale())
                continue;
            if (v1->length() <= 0) {
                if (pnt == 1)
                    GRpkgIf()->ErrPrintf(ET_WARN, Msg, v1->name());
                continue;
            }
            if (v1->isreal())
                TTY.printf_force("  %-10s %s\n", v1->name(), 
                    SPnum.printnum(v1->realval(0), v1->units(), false));

            else {
                // remember printnum returns static data!
                TTY.printf_force("  %-10s %s, ", v1->name(), 
                    SPnum.printnum(v1->realval(0), v1->units(), false));
                TTY.printf_force("%s\n",
                    SPnum.printnum(v1->imagval(0), v1->units(), false));
            }
        }
        sDvList::destroy(dvl);
    }
    return (true);
}


// Print the condition.  If fp is 0 and retstr is not 0,
// add the text to *retstr, otherwise print to fp;
//
void
sRunop::printcond(char **retstr)
{
    char buf[BSIZE_SP];
    const char *msg1 = " %s %d";
    const char *msg2 = " %s %s";
    const char *msg3 = " %s %g";

    sLstr lstr;
    if (retstr) {
        lstr.add(*retstr);
        delete [] *retstr;
        *retstr = 0;
    }
    for (sRunop *dt = this; dt; dt = dt->ro_also) {
        if (retstr) {
            if (dt->ro_type == RO_STOPAFTER) {
                if (dt->ro_ptmode)
                    sprintf(buf, msg1, kw_after, dt->ro_p.ipoint);
                else
                    sprintf(buf, msg3, kw_after, dt->ro_p.dpoint);
            }
            else if (dt->ro_type == RO_STOPAT) {
                if (dt->ro_ptmode)
                    sprintf(buf, msg1, kw_at, dt->ro_a.ipoints[dt->ro_index-1]);
                else
                    sprintf(buf, msg3, kw_at, dt->ro_a.dpoints[dt->ro_index-1]);
            }
            else if (dt->ro_type == RO_STOPBEFORE) {
                if (dt->ro_ptmode)
                    sprintf(buf, msg1, kw_before, dt->ro_p.ipoint);
                else
                    sprintf(buf, msg3, kw_before, dt->ro_p.dpoint);
            }
            else
                sprintf(buf, msg2, kw_when, dt->ro_string);
            lstr.add(buf);
        }
        else {
            if (dt->ro_type == RO_STOPAFTER) {
                if (dt->ro_ptmode)
                    TTY.printf(msg1, kw_after, dt->ro_p.ipoint);
                else
                    TTY.printf(msg3, kw_after, dt->ro_p.dpoint);
            }
            else if (dt->ro_type == RO_STOPAT) {
                if (dt->ro_ptmode)
                    TTY.printf(msg1, kw_at, dt->ro_a.ipoints[dt->ro_index - 1]);
                else
                    TTY.printf(msg3, kw_at, dt->ro_a.dpoints[dt->ro_index - 1]);
            }
            else if (dt->ro_type == RO_STOPBEFORE) {
                if (dt->ro_ptmode)
                    TTY.printf(msg1, kw_before, dt->ro_p.ipoint);
                else
                    TTY.printf(msg3, kw_before, dt->ro_p.dpoint);
            }
            else
                TTY.printf(msg2, kw_when, dt->ro_string);
        }
    }
    if (retstr) {
        lstr.add_c('\n');
        *retstr = lstr.string_trim();
    }
    else
        TTY.send("\n");
}
// End of sRunop functions.


void
sRunopDb::clear()
{
    sRunop::destroy_list(rd_iplot);
    sRunop::destroy_list(rd_trace);
    sRunop::destroy_list(rd_save);
    sRunop::destroy_list(rd_stop);
    sMeas::destroy(rd_meas);
    rd_iplot = 0;
    rd_trace = 0;
    rd_save = 0;
    rd_stop = 0;
    rd_meas = 0;
    rd_runopcnt = 1;
    rd_stepcnt = 0;
    rd_steps = 0;
}
// End of sRunopDb functions.

