
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#define TS_MAX_FSMS    16
#define TS_MAX_RULES   8192
#define TS_MAX_OBJS    8192


// 0 = off, 1 = log, 2 = abort, 3 = sample (abort every Nth), 4 = log+sample
static int   ts_mode = 2;      // default = abort
static int   ts_sample_N = 1000;
static int   ts_verbose = 0;

// 
struct ts_rule {
    int fsm; int src; const char* method; int dst;
};
static struct ts_rule ts_rules[TS_MAX_RULES];
static int ts_rules_n = 0;

struct ts_obj {
    void* ptr; int fsm; int state;
};
static struct ts_obj ts_objs[TS_MAX_OBJS];
static int ts_objs_n = 0;

static int ts_initial[TS_MAX_FSMS];        // per-FSM initial; 0 = infer
static int ts_initial_set[TS_MAX_FSMS];    // flags


static int parse_env_int(const char* name, int defv) {
    const char* s = getenv(name);
    if (!s || !*s) return defv;
    char* end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s) return defv;
    return (int)v;
}

static void ts_init_once(void) {
    static int inited = 0;
    if (inited) return;
    inited = 1;

    // Modes via env:
    //   TS_MODE=off|log|abort|sample
    //   TS_SAMPLE=N
    //   TS_VERBOSE=1
    const char* m = getenv("TS_MODE");
    if (m) {
        if (!strcmp(m, "off"))    ts_mode = 0;
        else if (!strcmp(m, "log"))   ts_mode = 1;
        else if (!strcmp(m, "abort")) ts_mode = 2;
        else if (!strcmp(m, "sample")) ts_mode = 3;
        else if (!strcmp(m, "logsample")) { ts_mode = 4; }
    }
    ts_sample_N = parse_env_int("TS_SAMPLE", ts_sample_N);
    ts_verbose  = parse_env_int("TS_VERBOSE", ts_verbose);
}

static int idx_obj(void* obj, int fsm) {
    for (int i = 0; i < ts_objs_n; ++i) {
        if (ts_objs[i].ptr == obj && ts_objs[i].fsm == fsm) return i;
    }
    if (ts_objs_n < TS_MAX_OBJS) {
        int i = ts_objs_n++;
        ts_objs[i].ptr  = obj;
        ts_objs[i].fsm  = fsm;
        ts_objs[i].state= 0;  
        return i;
    }
    // crude fallback: reuse last slot
    return TS_MAX_OBJS - 1;
}

static int compute_initial_state(int fsm) {
    // Choose “src with no incoming edges” for this FSM.
    int pick = 0;
    for (int i = 0; i < ts_rules_n; ++i) {
        if (ts_rules[i].fsm != fsm) continue;
        int cand = ts_rules[i].src, seen = 0;
        for (int j = 0; j < ts_rules_n; ++j) {
            if (ts_rules[j].fsm != fsm) continue;
            if (ts_rules[j].dst == cand) { seen = 1; break; }
        }
        if (!seen) return cand;
        if (!pick) pick = cand; // fallback: first src seen
    }
    return pick; // may be 0 if no rules
}

static int find_next_state(int fsm, int cur, const char* method, int* out) {
    for (int i = 0; i < ts_rules_n; ++i) {
        if (ts_rules[i].fsm == fsm && ts_rules[i].src == cur &&
            strcmp(ts_rules[i].method, method) == 0) {
            *out = ts_rules[i].dst;
            return 1;
        }
    }
    return 0;
}

static void handle_violation(void* obj, int fsm, int st, const char* method) {
    static unsigned long counter = 0;
    ++counter;

    if (ts_mode == 0) return; // off

    // decide whether to act this time
    int fire = 1;
    if (ts_mode == 3 || ts_mode == 4) { // sample
        if (ts_sample_N <= 1) fire = 1;
        else fire = (counter % (unsigned)ts_sample_N == 0);
    }

    if (!fire) return;

    if (ts_mode == 1 || ts_mode == 4) { // log
        fprintf(stderr, "[ts] INVALID fsm=%d obj=%p state=%d method=%s\n",
                fsm, obj, st, method);
        fflush(stderr);
        return;
    }

    // default: abort
    fprintf(stderr, "[ts] INVALID fsm=%d obj=%p state=%d method=%s — abort\n",
            fsm, obj, st, method);
    fflush(stderr);
    abort();
}



// Register a rule: for FSM fsm, from src --method--> dst
void __ts_init_rule_fsm(int fsm, int src, const char* method, int dst) {
    if (fsm < 0 || fsm >= TS_MAX_FSMS) return;
    if (ts_rules_n >= TS_MAX_RULES) return;
    ts_rules[ts_rules_n].fsm    = fsm;
    ts_rules[ts_rules_n].src    = src;
    ts_rules[ts_rules_n].method = method; // plugin passes literal
    ts_rules[ts_rules_n].dst    = dst;
    ts_rules_n++;
}

//  set initial for an FSM explicitly
void __ts_set_initial_fsm(int fsm, int s) {
    if (fsm < 0 || fsm >= TS_MAX_FSMS) return;
    ts_initial[fsm] = s;
    ts_initial_set[fsm] = 1;
}

//change mode programmatically (0 off, 1 log, 2 abort, 3 sample, 4 log+sample)
void __ts_set_mode(int mode, int sampleN) {
    ts_mode = mode;
    if (sampleN > 0) ts_sample_N = sampleN;
}

// Optional verbosity
void __ts_set_verbose(int v) { ts_verbose = v; }

// Main guard: called at unknown(unproven) callsites
// Decide/advance typestate dynamically using the actual object pointer.
void __ts_check_or_abort_fsm(void* obj, int fsm, const char* method) {
    ts_init_once();

    if (fsm < 0 || fsm >= TS_MAX_FSMS) return;
    if (ts_rules_n == 0) {
        static int warned = 0;
        if (!warned) { fprintf(stderr, "[ts] no rules registered; checks disabled\n"); warned = 1; }
        return;
    }

    int i = idx_obj(obj, fsm);
    int st = ts_objs[i].state;

    if (st == 0) {
        int init = ts_initial_set[fsm] ? ts_initial[fsm] : compute_initial_state(fsm);
        ts_objs[i].state = st = init;
        if (ts_verbose) fprintf(stderr, "[ts] init fsm=%d obj=%p state=%d\n", fsm, obj, st);
    }

    int next = -1;
    if (!find_next_state(fsm, st, method, &next)) {
        handle_violation(obj, fsm, st, method);
        return; // do not advance on invalid
    }

    if (ts_verbose) fprintf(stderr, "[ts] ok   fsm=%d obj=%p %d->%d via %s\n",
                             fsm, obj, st, next, method);
    ts_objs[i].state = next;
}



//dst now shares src's state for the same FSM.
void __ts_copy_fsm(void* dst, void* src, int fsm) {
    int is = idx_obj(src, fsm);
    int id = idx_obj(dst, fsm);
    ts_objs[id].state = ts_objs[is].state;
}

// Move: dst takes src's state, src reset (0 => re-init on next use)
void __ts_move_fsm(void* dst, void* src, int fsm) {
    int is = idx_obj(src, fsm);
    int id = idx_obj(dst, fsm);
    ts_objs[id].state = ts_objs[is].state;
    ts_objs[is].state = 0;
}

// Reset: placement new / destructor / external re-init
void __ts_reset_fsm(void* obj, int fsm) {
    int i = idx_obj(obj, fsm);
    ts_objs[i].state = 0;
}

#ifdef __cplusplus
}
#endif
