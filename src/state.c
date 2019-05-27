/*
 *  Copyright (C) 2003 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: state.c,v 1.73 2008/02/04 14:23:34 esaracco Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>

#include "config.h"
#include "main.h"
#include "callbacks.h"
#include "geq.h"
#include "spectrum.h"
#include "intrim.h"
#include "state.h"
#include "io.h"
#include "process.h"
#include "scenes.h"
#include "hdeq.h"
#include "compressor-ui.h"
#include "help.h"
#include "preferences.h"

/* A scene value to indicate that loading failed */
#define LOAD_ERROR -2


/* The smallest value that counts as a change, should be approximately
 * epsilon+delta */
#define MIN_CHANGE (FLT_EPSILON + FLT_EPSILON)

float                   s_value[S_SIZE];
static float            s_target[S_SIZE];
static int              s_duration[S_SIZE];
static int              s_changed[S_SIZE];
static GtkAdjustment   *s_adjustment[S_SIZE];
static s_callback_func  s_callback[S_SIZE];
static char             *errstr = NULL;


static s_state       *last_state = NULL;
static int	      last_changed = S_NONE;

static GList         *history = NULL;
static GList         *undo_pos = NULL;

static int suppress_feedback = 0;
static int saved_scene;
static float crossfade_time = 1.0;
static gboolean override_limiter_default = FALSE;

static void s_set_events(int id, float value);
void s_update_title();
void s_history_add(const char *description);
void s_save_global_int(xmlDocPtr doc, char *symbol, int value);
void s_save_global_float(xmlDocPtr doc, char *symbol, float value);
void s_save_global_gang(xmlDocPtr doc, char *p, int band, gboolean value);

gchar *session_filename = NULL;

/* global session parameters read from the XML file */

typedef struct {
    int scene;
    int mode;
    float freq;
    float lgain;
    float hgain;
    float ct;
    float inwl;
    float outwl;
    int eq_bypass;
    int comp_bypass[3];
    int limiter_bypass;
    gboolean out_meter_peak_pref;
    gboolean rms_meter_peak_pref;
    int rms_time_slice;
    int limiter_plugin;
    float xo_delay_time[XO_BANDS - 1];
    int xo_delay_state[XO_BANDS - 1];
    int gang_at[XO_BANDS];
    int gang_re[XO_BANDS];
    int gang_th[XO_BANDS];
    int gang_ra[XO_BANDS];
    int gang_kn[XO_BANDS];
    int gang_ma[XO_BANDS];
    int iir_xover;
} xml_global_params;

void state_init()
{
    unsigned int i;

    for (i=0; i<S_SIZE; i++) {
	s_value[i] = 0.0f;
	s_target[i] = 0.0f;
	s_duration[i] = 0;
	s_changed[i] = 0;
	s_adjustment[i] = NULL;
	s_callback[i] = NULL;
    }


    /*  Defaults for notches.  */

    s_value[S_NOTCH_Q(1)] = 5.0;
    s_value[S_NOTCH_Q(2)] = 5.0;
    s_value[S_NOTCH_Q(3)] = 5.0;

    s_value[S_NOTCH_FREQ(0)] = hdeq_get_notch_default_freq (0);
    s_value[S_NOTCH_FREQ(1)] = hdeq_get_notch_default_freq (1);
    s_value[S_NOTCH_FREQ(2)] = hdeq_get_notch_default_freq (2);
    s_value[S_NOTCH_FREQ(3)] = hdeq_get_notch_default_freq (3);
    s_value[S_NOTCH_FREQ(4)] = hdeq_get_notch_default_freq (4);


    s_history_add("Initial state");
}

void s_suppress_push()
{
    suppress_feedback++;
}

void s_suppress_pop()
{
    suppress_feedback--;
}

void s_set_callback(int id, s_callback_func callback)
{
    assert(id >= 0 && id < S_SIZE);

    s_callback[id] = callback;
}

void s_set_adjustment(int id, GtkAdjustment *adjustment)
{
    assert(id >= 0 && id < S_SIZE);

    s_adjustment[id] = adjustment;
}

void s_set_value_ui(int id, float value)
{
    s_value[id] = value;

    if (suppress_feedback) {
	return;
    }
    assert(id >= 0 && id < S_SIZE);


    if (last_changed != id) {
	s_history_add(g_strdup_printf("%s = %f", s_description[id],
		      s_value[id]));
    }
    last_state->value[id] = value;

#if 0
    /* This code is confusing in use, so I've removed it - swh */

    if (value - MIN_CHANGE < last_state->value[id] &&
	value + MIN_CHANGE > last_state->value[id]) {
	last_changed = S_NONE;
    } else {
	last_changed = id;
    }
#else
    last_changed = id;
#endif

    if (s_callback[id]) {
	(*s_callback[id])(id, value);
    }
}

void s_set_value(int id, float value, int duration)
{
    /* We dont want to call this yet... s_set_value_ui(id, value); */
    s_duration[id] = duration;
    s_target[id] = value;
    if (s_adjustment[id]) {
	gtk_adjustment_set_value(s_adjustment[id], value);
    }
}

void s_set_value_block(float *values, int base, int count)
{
    int i;

    for (i = 0 ; i < count ; i++) {
	s_value[base + i] = values[i];
    }
    last_changed = base;
    s_set_events(base, values[i]);
}

void s_set_value_no_history(int id, float value)
{
    suppress_feedback++;
    s_value[id] = value;
    s_target[id] = value;
    s_set_events(id, value);
    suppress_feedback--;
}

void s_clear_history()
{
    GList *p;

    for (p=history; p; p=p->next) {
	free(p->data);
    }
    g_list_free(history);
    history = NULL;
    s_history_add("Initial state");
    undo_pos = history->next;
    s_restore_state((s_state *)history->data);
    last_changed = S_LOAD;
}

void s_history_add(const char *description)
{
    s_state *ns;
    GList *it;

    ns = malloc(sizeof(s_state));
    ns->description = (char *)description;
    memcpy(ns->value, s_value, S_SIZE * sizeof(float));

    if (undo_pos) {
	it = undo_pos->next;
	while (it) {
	    free(it->data);
	    it = it->next;
	}
	undo_pos->next = NULL;
    }

    history = g_list_append(history, ns);
    undo_pos = g_list_last(history);
    /* printf("add %s\n", description); */
    last_state = ns;
}

void s_history_add_state(s_state state)
{
    s_state *ns;
    GList *it;

    ns = malloc(sizeof(s_state));
    ns->description = (char *)state.description;
    memcpy(ns->value, state.value, S_SIZE * sizeof(float));

    if (undo_pos) {
	it = undo_pos->next;
	while (it) {
	    free(it->data);
	    it = it->next;
	}
	undo_pos->next = NULL;
    }

    history = g_list_append(history, ns);
    undo_pos = g_list_last(history);
    /* printf("add %s\n", description); */
    last_state = ns;
}

static unsigned int compute_state_crc (s_state *state)
{
    unsigned int        checksum, i;
    unsigned char       *buf;
    unsigned int        crc_table[256] = 
      {0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,
       0xE963A535,0x9E6495A3,0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,
       0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,0x1DB71064,0x6AB020F2,
       0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
       0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,
       0xFA0F3D63,0x8D080DF5,0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,
       0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,
       0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
       0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,
       0xCFBA9599,0xB8BDA50F,0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,
       0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,0x76DC4190,0x01DB7106,
       0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
       0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,
       0x91646C97,0xE6635C01,0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,
       0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,
       0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
       0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,
       0xA4D1C46D,0xD3D6F4FB,0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,
       0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,0x5005713C,0x270241AA,
       0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
       0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,
       0xB7BD5C3B,0xC0BA6CAD,0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,
       0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,0xE3630B12,0x94643B84,
       0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
       0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,
       0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,
       0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,0xD6D6A3E8,0xA1D1937E,
       0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
       0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,
       0x316E8EEF,0x4669BE79,0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,
       0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,0xC5BA3BBE,0xB2BD0B28,
       0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
       0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,
       0x72076785,0x05005713,0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,
       0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,0x86D3D2D4,0xF1D4E242,
       0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
       0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,
       0x616BFFD3,0x166CCF45,0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,
       0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,0xAED16A4A,0xD9D65ADC,
       0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
       0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,
       0x54DE5729,0x23D967BF,0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,
       0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D};


    checksum = ~0;

    buf = (unsigned char *) state->value;

    for (i = 0 ; i < S_SIZE * sizeof (float) ; i++) 
      checksum = crc_table[(checksum ^ buf[i]) & 0xff] ^ (checksum >> 8);

    checksum ^= ~0;


    return (checksum);
}

void s_undo() 
{
    GList *undo_next;
    int       scene, crc[2];
    s_state   *st[2];


    if (!undo_pos) {
	return;
    }
    undo_next = g_list_previous(undo_pos);
    if (!undo_next) {
	return;
    }
    undo_pos = undo_next;
    s_restore_state((s_state *)undo_pos->data);

    scene = get_previous_scene_num ();
    if (scene >= 0)
      {
        st[0] = get_scene (scene);
        st[1] = (s_state *) undo_pos->data;

        crc[0] = compute_state_crc (st[0]);
        crc[1] = compute_state_crc (st[1]);

        if (crc[0] == crc[1]) set_scene_button (scene);
      }

    set_EQ_curve_values (0, 0.0);
    last_changed = S_LOAD;
}

void s_redo() 
{
    gboolean  restore;
    int       scene, crc[2];
    s_state   *st[2];


    restore = FALSE;
    if (undo_pos) {
      if (undo_pos->next) {
        undo_pos = g_list_next(undo_pos);
        restore = TRUE;
      }
    } else if (history->next) {
      undo_pos = history;
      undo_pos = g_list_next(undo_pos);
      restore = TRUE;
    }

    if (restore)
      {
        s_restore_state((s_state *)undo_pos->data);

        scene = get_previous_scene_num ();
        if (scene >= 0)
          {
            st[0] = get_scene (scene);
            st[1] = (s_state *) undo_pos->data;

            crc[0] = compute_state_crc (st[0]);
            crc[1] = compute_state_crc (st[1]);

            if (crc[0] == crc[1]) 
              {
                set_scene_button (scene);
              }
            else
              {
                set_scene_warning_button (scene);
              }
          }
        set_EQ_curve_values (0, 0.0);
      }
}


/*  Negative time will use the default "crossfade time" which may come from the
    command line -c option.  */

void s_crossfade_to_state(s_state *state, float time)
{
    int i, duration, milliseconds;

    if (time < 0.0) time = crossfade_time;


    suspend_ganging ();
    milliseconds = time * 1000.0 + 50;
    g_timeout_add (milliseconds, unsuspend_ganging, NULL);


    /* printf("restore %s\n", state->description); */
    duration = (int)(sample_rate * time);
    suppress_feedback++;
    for (i=0; i<S_SIZE; i++) {
	/* set the target and duration for crosssfade, but set the controls to
	 * the endpoint */
	s_target[i] = state->value[i];
	s_duration[i] = duration;
	s_set_events(i, state->value[i]);
    }
    suppress_feedback--;
}

void s_restore_state(s_state *state)
{
    s_crossfade_to_state (state, 0.003);
}

static void s_set_events(int id, float value)
{
    if (s_callback[id]) {
	(*s_callback[id])(id, value);
    }
    if (s_adjustment[id]) {
	gtk_adjustment_set_value(s_adjustment[id], value);
    }
}

void s_set_description(int id, const char *desc)
{
    if (last_changed != id) {
	s_history_add(desc);
    }
    last_changed = id;
}

void s_save_session_from_ui (GtkWidget *w, gpointer user_data)
{
#if GTK_VERSION_GE(2, 4)

    gchar *fname = NULL;
    GtkFileChooser *file_selector = (GtkFileChooser *) user_data;

    fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_selector));
    s_save_session (fname);
    g_free (fname);

#else

    GtkFileSelection *file_selector = (GtkFileSelection *) user_data;

    s_save_session(gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_selector)));

#endif
}
    
void s_save_session (const gchar *fname)
{
    xmlDocPtr doc;
    xmlNodePtr rootnode, node, sc_node;
    unsigned int i, j;
    int curr_scene;
    char tmp[256];


    /* Check to see if we have been passed a filename, if not fall back to
     * previous one */

    if (fname) {
        s_set_session_filename (fname);
	s_update_title();
    }
    if (!s_have_session_filename ()) {
	errstr = g_strdup_printf("No session filename found at %s:%d, not saving\n",
                                 __FILE__, __LINE__);
	message (GTK_MESSAGE_WARNING, errstr);
	free(errstr);
    }


    /*  Need to save this scene number.  */

    curr_scene = get_current_scene ();

    xmlSetCompressMode(5);
    doc = xmlNewDoc("1.0");
    rootnode = xmlNewDocRawNode(doc, NULL, "jam-param-list", NULL);
    xmlSetProp(rootnode, "version", VERSION);
    xmlDocSetRootElement(doc, rootnode);
    node = xmlNewText("\n");
    xmlAddChild(rootnode, node);


    s_save_global_int(doc, "mode", process_get_spec_mode());
    s_save_global_int(doc, "freq", get_spectrum_freq());
    s_save_global_float(doc, "ct", crossfade_time);
    s_save_global_float(doc, "inwl", intrim_inmeter_get_warn());
    s_save_global_float(doc, "outwl", intrim_outmeter_get_warn());
    s_save_global_float(doc, "lgain", hdeq_get_lower_gain());
    s_save_global_float(doc, "hgain", hdeq_get_upper_gain());

    s_save_global_int(doc, "eq bypass", process_get_bypass_state (EQ_BYPASS));
    s_save_global_int(doc, "comp bypass0", process_get_bypass_state (LOW_COMP_BYPASS));
    s_save_global_int(doc, "comp bypass1", process_get_bypass_state (MID_COMP_BYPASS));
    s_save_global_int(doc, "comp bypass2", process_get_bypass_state (HIGH_COMP_BYPASS));
    s_save_global_int(doc, "limiter bypass", process_get_bypass_state (LIMITER_BYPASS));

    s_save_global_int(doc, "output meter peak pref", (int) intrim_get_out_meter_peak_pref ());
    s_save_global_int(doc, "rms meter peak pref", (int) intrim_get_rms_meter_peak_pref ());

    s_save_global_int(doc, "rms time slice", process_get_rms_time_slice ());

    s_save_global_int(doc, "limiter plugin", process_get_limiter_plugin ());


    s_save_global_float(doc, "low delay time", process_get_xo_delay_time(XO_LOW));
    s_save_global_int(doc, "low delay state", process_get_xo_delay_state (XO_LOW));
    s_save_global_float(doc, "mid delay time", process_get_xo_delay_time(XO_MID));
    s_save_global_int(doc, "mid delay state", process_get_xo_delay_state (XO_MID));

    s_save_global_int (doc, "crossover type", process_get_crossover_type ());

    /* record the current gang state of the compressor controls  */

    for (i = 0 ; i < XO_BANDS ; i++) {
        s_save_global_gang(doc, "at", i, comp_at_ganged(i));
        s_save_global_gang(doc, "re", i, comp_re_ganged(i));
        s_save_global_gang(doc, "th", i, comp_th_ganged(i));
        s_save_global_gang(doc, "ra", i, comp_ra_ganged(i));
        s_save_global_gang(doc, "kn", i, comp_kn_ganged(i));
        s_save_global_gang(doc, "ma", i, comp_ma_ganged(i));
    }


    /* Save current active state */

    for (i=0; i<S_SIZE; i++) {
	node = xmlNewDocRawNode(doc, NULL, "parameter", NULL);
	snprintf(tmp, 255, "%g", s_value[i]);
	xmlSetProp(node, "name", s_symbol[i]);
	xmlSetProp(node, "value", tmp);
	xmlAddChild(rootnode, node);
	node = xmlNewText("\n");
	xmlAddChild(rootnode, node);
    }


    /* Save scenes */

    for (j=0; j<NUM_SCENES; j++) {
	s_state *st = get_scene(j);
	sc_node = xmlNewDocRawNode(doc, NULL, "scene", NULL);
	snprintf(tmp, 255, "%d", j);
	xmlSetProp(sc_node, "number", tmp);
	if (!st) {
	    xmlAddChild(rootnode, sc_node);
	    node = xmlNewText("\n");
	    xmlAddChild(rootnode, node);
	    continue;
	}
	xmlSetProp(sc_node, "name", get_scene_name(j));
	if (curr_scene == j) {
	    xmlSetProp(sc_node, "active", "true");
	    xmlSetProp(sc_node, "changed", "false");
	} else if (curr_scene == changed_scene_no(j)) {
	    xmlSetProp(sc_node, "active", "true");
	    xmlSetProp(sc_node, "changed", "true");
	}
	node = xmlNewText("\n");
	xmlAddChild(sc_node, node);
	xmlAddChild(rootnode, sc_node);
	node = xmlNewText("\n");
	xmlAddChild(rootnode, node);

	for (i=0; i<S_SIZE; i++) {
	    node = xmlNewDocRawNode(doc, NULL, "parameter", NULL);
	    snprintf(tmp, 255, "%g", st->value[i]);
	    xmlSetProp(node, "name", s_symbol[i]);
	    xmlSetProp(node, "value", tmp);
	    xmlAddChild(sc_node, node);
	    node = xmlNewText("\n");
	    xmlAddChild(sc_node, node);
	}
    }
    xmlSaveFile((const char *) s_get_session_filename (), doc);
    xmlFreeDoc(doc);
}

/* declare SAX handlers */
void s_startElement(void *user_data, const xmlChar *name,
                    const xmlChar **attrs);

static void s_warning(void *user_data, const char *msg, ...);

static void s_error(void *user_data, const char *msg, ...);


void s_load_session_from_ui (GtkWidget *w, gpointer user_data)
{
#if GTK_VERSION_GE(2, 4)

    gchar *fname = NULL;
    GtkFileChooser *file_selector = (GtkFileChooser *) user_data;

    fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_selector));
    s_load_session (fname);
    g_free (fname);

#else

    GtkFileSelection *file_selector = (GtkFileSelection *) user_data;

    s_load_session(gtk_file_selection_get_filename (GTK_FILE_SELECTION
                                                (file_selector)));

#endif
}
    
void s_load_session (const gchar *fname)
{
    xmlSAXHandlerPtr handler;
    int scene = -1;
    int fd;
    int i;
    xml_global_params gp;
    gchar *session_filename;

    saved_scene = -1;
    unset_scene_buttons ();

    if (fname) {
	s_set_session_filename (fname);
    }
    if (!s_have_session_filename ()) {
	s_set_session_filename (default_session);
    }

    session_filename = s_get_session_filename ();

    /* Check to see if file is readable */
    if ((fd = open((const char *) session_filename, O_RDONLY)) >= 0) {
	close(fd);
    } else {
	errstr = g_strdup_printf("Error opening '%s'", session_filename);
	message (GTK_MESSAGE_WARNING, errstr);
        perror(errstr);
	free(errstr);

	if (fname == NULL) {
	    exit(1);
	}
    }

    s_update_title();

    handler = calloc(1, sizeof(xmlSAXHandler));
    handler->startElement = s_startElement;
    handler->warning = s_warning;
    handler->error = s_error;

    /* set the gp struct to some sensible defaults in case values aren't set in
     * the XML file */
    gp.scene = scene;
    gp.mode = SPEC_POST_EQ;
    gp.freq = 10;
    gp.lgain = -12.0;
    gp.hgain = 12.0;
    gp.inwl = -6.0;
    gp.outwl = -6.0;
    gp.ct = 1.0;
    gp.eq_bypass = 0;
    gp.comp_bypass[0] = gp.comp_bypass[1] = gp.comp_bypass[2] = 0;
    gp.limiter_bypass = 0;
    gp.out_meter_peak_pref = TRUE;
    gp.rms_meter_peak_pref = TRUE;
    gp.rms_time_slice = 50;
    gp.limiter_plugin = FAST;
    gp.xo_delay_time[XO_LOW] = 2.0;
    gp.xo_delay_state[XO_LOW] = 0;
    gp.xo_delay_time[XO_MID] = 0.5;
    gp.xo_delay_state[XO_MID] = 0;
    gp.iir_xover = FFT;
    for (i = 0 ; i < XO_BANDS ; i++) {
        gp.gang_at[i] = FALSE;
        gp.gang_re[i] = FALSE;
        gp.gang_th[i] = FALSE;
        gp.gang_ra[i] = FALSE;
        gp.gang_kn[i] = FALSE;
        gp.gang_ma[i] = FALSE;
    }


    /* run the SAX parser */    
    scene_init();
    xmlSAXUserParseFile(handler, &gp, (const char *) session_filename);

    if (gp.scene == LOAD_ERROR) {
	errstr = g_strdup_printf("Loading file '%s' failed", session_filename);
	message (GTK_MESSAGE_WARNING, errstr);
        perror(errstr);
	free(errstr);
	return;
    }

    s_history_add(g_strdup_printf("Load %s", session_filename));
    last_changed = S_LOAD;
    free(handler);

    /* global params are read into the gp struct, and set in the UI code from
     * here */

    process_set_spec_mode (gp.mode);
    set_spectrum_freq (gp.freq);
    hdeq_set_upper_gain (gp.hgain);
    geq_set_range (gtk_adjustment_get_lower(geq_get_adjustment(0)), gp.hgain);
    hdeq_set_lower_gain (gp.lgain);
    geq_set_range (gp.lgain, gtk_adjustment_get_upper(geq_get_adjustment(0)));
    s_set_crossfade_time (gp.ct);
    intrim_inmeter_set_warn (gp.inwl);
    intrim_outmeter_set_warn (gp.outwl);


    for (i = 0 ; i < XO_NBANDS ; i++)
      {
        if (gp.comp_bypass[i] != 2)
          {
            callbacks_set_comp_bypass_button_state (i, FALSE);
          }
        else
          {
            callbacks_set_comp_bypass_button_state (i, TRUE);
          }
      }

    callbacks_set_eq_bypass_button_state (gp.eq_bypass);
    callbacks_set_limiter_bypass_button_state (gp.limiter_bypass);

    process_set_xo_delay_time (XO_LOW, gp.xo_delay_time[XO_LOW]);
    process_set_xo_delay_time (XO_MID, gp.xo_delay_time[XO_MID]);
    process_set_xo_delay_state (XO_LOW, gp.xo_delay_state[XO_LOW]);
    process_set_xo_delay_state (XO_MID, gp.xo_delay_state[XO_MID]);

    callbacks_set_low_delay_button_state (gp.xo_delay_state[XO_LOW]);
    callbacks_set_mid_delay_button_state (gp.xo_delay_state[XO_MID]);

    intrim_set_out_meter_peak_pref (gp.out_meter_peak_pref);
    intrim_set_rms_meter_peak_pref (gp.rms_meter_peak_pref);

    process_set_rms_time_slice (gp.rms_time_slice);

    process_set_crossover_type (gp.iir_xover);

    /*  This is the active scene.  */

    if (saved_scene < 100)
      {
        set_scene (saved_scene);
      }
    else
      {
        set_num_scene_warning_button(saved_scene);
      }

    if (!fname) {
	s_set_session_filename (NULL);
    }


    /*  Set the ganging after the scene otherwise it will try to gang move
        the scene values.  */

    for (i = 0 ; i < XO_BANDS ; i++) {
        if (gp.gang_at[i]) comp_gang_at (i);
        if (gp.gang_re[i]) comp_gang_re (i);
        if (gp.gang_th[i]) comp_gang_th (i);
        if (gp.gang_ra[i]) comp_gang_ra (i);
        if (gp.gang_kn[i]) comp_gang_kn (i);
        if (gp.gang_ma[i]) comp_gang_ma (i);
    }


    /*  If we set the limiter plugin on the command line don't set
        it from the defaults.  */

    if (!override_limiter_default) process_set_limiter_plugin (gp.limiter_plugin);
    override_limiter_default = FALSE;

	if(gui_mode == 0) // Default mode
		hdeq_set_xover ();
    set_EQ_curve_values (0, 0.0);

    s_clear_history();
    pref_set_all_values ();
}

void s_startElement(void *user_data, const xmlChar *name, const xmlChar **attrs)
{
    const xmlChar **p;
    unsigned int i, found = 0;
    const char *symbol = NULL, *value = NULL, *index = NULL;
    xml_global_params *gp = user_data;
    int active = 0;
    int changed = 0;

    if (!strcmp(name, "jam-param-list")) {
	return;
    }

    if (!strcmp(name, "scene")) {
	const char *sname = NULL;

	for (p=attrs; p && *p; p+=2) {
	    if (!strcmp(*p, "name")) {
		sname = *(p+1);
	    } else if (!strcmp(*p, "number")) {
		gp->scene = atoi(*(p+1));
	    } else if (!strcmp(*p, "active") && !strcmp(*(p+1), "true")) {
		active = 1;
	    } else if (!strcmp(*p, "changed") && !strcmp(*(p+1), "true")) {
		changed = 1;
	    }
	}


        /*  Set the active scene number to be set after parsing all of the
            scenes in the XML file.  */

	if (active) {
            saved_scene = gp->scene;
	    set_scene(gp->scene);
	}
	if (changed) {
            saved_scene = gp->scene + 100;
	    set_num_scene_warning_button(changed_scene_no(gp->scene));
	}

	if (sname && gp->scene > -1) {
	    set_scene(gp->scene);
	    set_scene_name(gp->scene, sname);
	}

	return;
    }

    /* if its a global setting */
    if (!strcmp(name, "global")) {
	/* find the name, value and index attributes */
	for (p=attrs; p && *p; p+=2) {
	    if (!strcmp(*p, "name")) {
		symbol = *(p+1);
	    } else if (!strcmp(*p, "value")) {
		value = *(p+1);
	    } else if (!strcmp(*p, "index")) {
		index = *(p+1);
	    }
	}

	if (!strcmp(symbol, "mode")) {
	    gp->mode = atoi(value);
	} else if (!strcmp(symbol, "freq")) {
	    gp->freq = atof(value);
	} else if (!strcmp(symbol, "lgain")) {
	    gp->lgain = atof(value);
	} else if (!strcmp(symbol, "hgain")) {
	    gp->hgain = atof(value);
	} else if (!strcmp(symbol, "ct")) {
	    gp->ct = atof(value);
	} else if (!strcmp(symbol, "inwl")) {
	    gp->inwl = atof(value);
	} else if (!strcmp(symbol, "outwl")) {
	    gp->outwl = atof(value);
	} else if (!strcmp(symbol, "eq bypass")) {
	    gp->eq_bypass = atof(value);
	} else if (!strcmp(symbol, "comp bypass0")) {
	    gp->comp_bypass[0] = atoi(value);
	} else if (!strcmp(symbol, "comp bypass1")) {
	    gp->comp_bypass[1] = atoi(value);
	} else if (!strcmp(symbol, "comp bypass2")) {
	    gp->comp_bypass[2] = atoi(value);
	} else if (!strcmp(symbol, "limiter bypass")) {
	    gp->limiter_bypass = atoi(value);
	} else if (!strcmp(symbol, "output meter peak pref")) {
          gp->out_meter_peak_pref = (gboolean) atoi(value);
	} else if (!strcmp(symbol, "rms meter peak pref")) {
          gp->rms_meter_peak_pref = (gboolean) atoi(value);
	} else if (!strcmp(symbol, "rms time slice")) {
          gp->rms_time_slice = atoi(value);
	} else if (!strcmp(symbol, "limiter plugin")) {
          gp->limiter_plugin = atoi(value);
	} else if (!strcmp(symbol, "low delay time")) {
          gp->xo_delay_time[XO_LOW] = atof(value);
	} else if (!strcmp(symbol, "mid delay time")) {
          gp->xo_delay_time[XO_MID] = atof(value);
	} else if (!strcmp(symbol, "low delay state")) {
          gp->xo_delay_state[XO_LOW] = atoi(value);
	} else if (!strcmp(symbol, "mid delay state")) {
          gp->xo_delay_state[XO_MID] = atoi(value);
	} else if (!strcmp(symbol, "crossover type")) {
          gp->iir_xover = atoi(value);
	} else if (!strcmp(symbol, "mid delay state")) {
	} else if ((const char *)strstr(symbol, "gang_") == symbol) {
	    int ind = index ? atoi(index) : -1;
	    int val = atoi(value);

	    if (ind >= 0 && ind < XO_BANDS) {
		if (!strcmp(symbol, "gang_at")) {
		    gp->gang_at[ind] = val;
		} else if (!strcmp(symbol, "gang_re")) {
		    gp->gang_re[ind] = val;
		} else if (!strcmp(symbol, "gang_th")) {
		    gp->gang_th[ind] = val;
		} else if (!strcmp(symbol, "gang_ra")) {
		    gp->gang_ra[ind] = val;
		} else if (!strcmp(symbol, "gang_kn")) {
		    gp->gang_kn[ind] = val;
		} else if (!strcmp(symbol, "gang_ma")) {
		    gp->gang_ma[ind] = val;
		} else {
		    errstr = g_strdup_printf("Unhandled gang: %s\n", symbol);
		    message (GTK_MESSAGE_WARNING, errstr);
		    free(errstr);
		}
	    } else {
		errstr = g_strdup_printf("Unhandled index: %s\n", index);
		message (GTK_MESSAGE_WARNING, errstr);
		free(errstr);
	    }
	} else {
	    errstr = g_strdup_printf("Unhandled global parameter: %s\n", symbol);
	    message (GTK_MESSAGE_WARNING, errstr);
	    free(errstr);
	}

	return;
    }

    /* Check its a parameter element */
    if (strcmp(name, "parameter")) {
	errstr = g_strdup_printf("Unhandled element: %s\n", name);
	message (GTK_MESSAGE_WARNING, errstr);
	free(errstr);
    }

    /* Find the name and value attributes */
    for (p=attrs; p && *p; p+=2) {
	if (!strcmp(*p, "name")) {
	    symbol = *(p+1);
	} else if (!strcmp(*p, "value")) {
	    value = *(p+1);
	}
    }

    /* Find the matching symbol, this is horribly inefficient */
    for (i=0; i<S_SIZE && !found; i++) {
	if (!strcmp(symbol, s_symbol[i])) {
	    if (gp->scene == -1) {
		s_value[i] = atof(value);
		suppress_feedback++;
		s_set_events(i, s_value[i]);
		suppress_feedback--;
	    } else {
		s_state *st = get_scene(gp->scene);
		if (st) {
		    st->value[i] = atof(value);
		} else {
                  errstr = g_strdup_printf("Bad scene number %d\n", gp->scene);
                  message (GTK_MESSAGE_WARNING, errstr);
                  free(errstr);
		}
	    }
	    //printf("load %s = %g\n", symbol, s_value[i]);
	    found = 1;
	    break;
	}
    }
    if (!found) {

      /*  We need to disregard stereo-balance settings as these have been removed
          but may still be in old .jam files.  */

      if (!strstr (symbol ,"stereo-balance"))
        {
	  errstr = g_strdup_printf("Unknown symbol: %s in element %s\n",
				   symbol, name);
          message (GTK_MESSAGE_WARNING, errstr);
          free(errstr);
        }
    }
}

static void s_warning(void *user_data, const char *msg, ...)
{
    va_list args;
    char *fmt;

    va_start(args, msg);
    fmt = g_strdup_printf("XML parser warning: %s", msg);
    vfprintf(stderr, fmt, args);
    free(fmt);
    va_end(args);
}

static void s_error(void *user_data, const char *msg, ...)
{
    va_list args;
    char *fmt;

    va_start(args, msg);
    fmt = g_strdup_printf("XML parser error: %s", msg);
    vfprintf(stderr, fmt, args);
    free(fmt);
    va_end(args);
    *((int *)user_data) = LOAD_ERROR;
}

void s_crossfade(const int nframes)
{
    unsigned int i;


    for (i=0; i<S_SIZE; i++) {
        if (s_duration[i] != 0) {
/* debug crap if (i == S_IN_GAIN) printf("%d\t%f\n", s_duration[i], s_value[i]); */
            s_duration[i] -= nframes;
            if (s_duration[i] > nframes) {
                s_value[i] += ((float)nframes / (float)s_duration[i]) *
                                (s_target[i] - s_value[i]);
            } else {
                s_value[i] = s_target[i];
                s_duration[i] = 0;
            }
	    s_changed[i] = 1;
        }
    }
}

void s_crossfade_ui()
{
    unsigned int i;


    suppress_feedback++;
    for (i=0; i<S_SIZE; i++) {
	if (s_changed[i]) {
	    s_changed[i] = 0;
	    s_set_events(i, s_value[i]);
	}
    }
    suppress_feedback--;
}

int s_have_session_filename()
{
    return (session_filename != NULL);
}

gchar *s_get_session_filename()
{
    return ((gchar *) session_filename);
}

void s_update_title()
{
    char *title; 
    char *base;
    gchar *tmp;

    /* name for title bar */
    char *title_name = (client_name? client_name: PACKAGE);

    tmp = g_strdup (s_get_session_filename ());
    base = basename (tmp);
    title = g_strdup_printf ("%s - %s - " VERSION, title_name, base);
    g_free (tmp);
    gtk_window_set_title ((GtkWindow *) main_window, title);
    g_free (title);
}

void s_set_session_filename(const gchar *fname)
{
    if (session_filename != NULL)
      g_free (session_filename);

    if (fname != NULL) {
      session_filename = g_strdup (fname);
    } else {
      session_filename = NULL;
    }
}

void s_set_crossfade_time(float ct)
{
  /*  We're faking them out here.  0.0 isn't really allowable but
      most people would rather put in 0.0 instead of 0.001.  */

  if (ct == 0.0) ct = 0.001;
  crossfade_time = ct;
}

float s_get_crossfade_time()
{
  return (crossfade_time);
}

void s_save_global_int(xmlDocPtr doc, char *symbol, int value)
{
    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr node = xmlNewDocRawNode(doc, NULL, "global", NULL);
    char tmp[256];

    snprintf(tmp, 255, "%d", value);
    xmlSetProp(node, "name", symbol);
    xmlSetProp(node, "value", tmp);
    xmlAddChild(root, node);
    node = xmlNewText("\n");
    xmlAddChild(root, node);
}

void s_save_global_float(xmlDocPtr doc, char *symbol, float value)
{
    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr node = xmlNewDocRawNode(doc, NULL, "global", NULL);
    char tmp[256];

    snprintf(tmp, 255, "%f", value);
    xmlSetProp(node, "name", symbol);
    xmlSetProp(node, "value", tmp);
    xmlAddChild(root, node);
    node = xmlNewText("\n");
    xmlAddChild(root, node);
}

void s_save_global_gang(xmlDocPtr doc, char *p, int band, gboolean value)
{
    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr node = xmlNewDocRawNode(doc, NULL, "global", NULL);
    char tmp[256];

    snprintf(tmp, 255, "gang_%s", p);
    xmlSetProp(node, "name", tmp);
    snprintf(tmp, 255, "%d", band);
    xmlSetProp(node, "index", tmp);
    snprintf(tmp, 255, "%d", value);
    xmlSetProp(node, "value", tmp);
    xmlAddChild(root, node);
    node = xmlNewText("\n");
    xmlAddChild(root, node);
}


/*  Set a boolean if we put the limiter in on the command line.  */

void s_set_override_limiter_default ()
{
  override_limiter_default = TRUE;
}


/* vi:set ts=8 sts=4 sw=4: */
