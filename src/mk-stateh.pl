#!/usr/bin/perl -w

open(SRC, "state-vars.txt") || die "Can't open 'state-vars.txt'";
open(OUT, "| indent -kr > state.h") || die "Can't invoke indent and write";

my @symbols;
my %card;
my %desc;
my $id;

$id = 0;

while(<SRC>) {
	next if (/^#/);
	if (/([a-z-]+)\t+(.+)/) {
		push @symbols, $1;
		$card{$1} = 1;
		$desc{$1} = $2;
	} elsif (/([a-z-]+)\((\d+)\)\t+(.+)/) {
		push @symbols, $1;
		$card{$1} = $2;
		$desc{$1} = $3;
	} else {
		die "Syntax error: $_";
	}
}

print OUT <<EOB;
#ifndef STATE_H
#define STATE_H

/* This is a generated file DO NOT EDIT, see state-vars.txt */

#include <gtk/gtk.h>

typedef void(* s_callback_func)(int id, float value);

typedef struct {
	int id;
	float value;
} s_entry;

void state_init();
void s_set_value_ui(int id, float value);
void s_set_value(int id, float value, int time);
void s_set_value_block(float *values, int base, int count);
void s_set_value_no_history(int id, float value);
void s_set_description(int id, const char *desc);
void s_clear_history();
void s_set_callback(int id, s_callback_func callback);
void s_set_adjustment(int id, GtkAdjustment *adjustment);
void s_history_add(const char *description);
void s_undo();
void s_redo();
void s_save_session_from_ui (GtkWidget *w, gpointer user_data);
void s_save_session (const char *fname);
void s_load_session_from_ui (GtkWidget *w, gpointer user_data);
void s_load_session (const char *fname);
void s_set_session_filename(const gchar *fname);
void s_crossfade(const int nframes);
void s_crossfade_ui();
void s_suppress_push();
void s_set_crossfade_time(float ct);
float s_get_crossfade_time();
void s_suppress_pop();
int s_have_session_filename();
gchar *s_get_session_filename();
void s_set_override_limiter_default ();



#define S_NONE -1
EOB

for $sym (@symbols) {
	$macro = $sym;
	$macro =~ s/-/_/g;
	$macro = "\U$macro";
	if ($card{$sym} == 1) {
		print OUT "#define S_$macro ".$id++."\n";
	} else {
		print OUT "#define S_$macro(n) ($id + n)\n";
		$id += $card{$sym};
	}
}

print OUT <<EOB;
#define S_SIZE $id

typedef struct {
    char *description;
    float value[S_SIZE];
} s_state;

void s_restore_state(s_state *state);
void s_crossfade_to_state(s_state *state, float time);
void s_history_add_state(s_state state);

extern float s_value[S_SIZE];

/* fetch currently used value */

inline static float s_get_value(int id)
{
	return s_value[id];
}

/* set value with no side effects */

inline static void s_set_value_ns(int id, float value)
{
	s_value[id] = value;
}

EOB

$first = 1;
print OUT "static const char * const s_description[S_SIZE] = {\n";
for $sym (@symbols) {
	if ($card{$sym} > 1) {
		for ($i = 1; $i <= $card{$sym}; $i++) {
			if (!$first) {
				print OUT ",";
			} else {
				$first = 0;
			}
			print OUT "\"$desc{$sym} $i\"";
		}
	} else {
		if (!$first) {
			print OUT ",";
		} else {
			$first = 0;
		}
		print OUT "\"$desc{$sym}\"";
	}
}
print OUT "};\n\n";

$first = 1;
print OUT "static const char * const s_symbol[S_SIZE] = {\n";
for $sym (@symbols) {
	if ($card{$sym} > 1) {
		for ($i = 0; $i < $card{$sym}; $i++) {
			if (!$first) {
				print OUT ",";
			} else {
				$first = 0;
			}
			print OUT "\"$sym$i\"";
		}
	} else {
		if (!$first) {
			print OUT ",";
		} else {
			$first = 0;
		}
		print OUT "\"$sym\"";
	}
}
print OUT "};\n\n";
print OUT "#endif\n";

close OUT;
