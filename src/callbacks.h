#include <gtk/gtk.h>


void
on_low2mid_value_changed               (GtkRange        *range,
                                        gpointer         user_data);

void
on_mid2high_value_changed              (GtkRange        *range,
                                        gpointer         user_data);

void
on_quit_button_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_window1_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_in_trim_scale_value_changed         (GtkRange        *range,
                                        gpointer         user_data);

void
on_window1_show                        (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_EQ_curve_configure_event            (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_EQ_curve_draw               (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         data);

void
on_EQ_curve_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_EQ_curve_event_box_motion_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_EQ_curve_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_EQ_curve_event_box_button_release_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_comp_kn_1_value_changed             (GtkRange        *range,
                                        gpointer         user_data);

void
on_comp_kn_2_value_changed             (GtkRange        *range,
                                        gpointer         user_data);

void
on_comp_kn_3_value_changed             (GtkRange        *range,
                                        gpointer         user_data);

void
on_lim_out_trim_scale_value_changed        (GtkRange        *range,
                                        gpointer         user_data);

void
on_pan_scale_value_changed             (GtkRange        *range,
                                        gpointer         user_data);

gboolean
on_comp1_curve_draw            (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         data);

void
on_comp1_curve_realize                 (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_comp2_curve_draw            (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         data);

void
on_comp2_curve_realize                 (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_comp3_curve_draw            (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         data);

void
on_comp3_curve_realize                 (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_low_curve_box_motion_notify_event   (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_mid_curve_box_motion_notify_event   (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_high_curve_box_motion_notify_event  (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_low_curve_box_leave_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_mid_curve_box_leave_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_high_curve_box_leave_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_low_curve_box_leave_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_low_curve_box_motion_notify_event   (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_mid_curve_box_leave_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_mid_curve_box_motion_notify_event   (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_high_curve_box_leave_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_high_curve_box_motion_notify_event  (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_EQ_curve_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_EQ_curve_event_box_button_release_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_EQ_curve_event_box_leave_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_EQ_curve_event_box_motion_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_low_curve_box_enter_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_mid_curve_box_enter_notify_event    (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_high_curve_box_enter_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_low_comp_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_low_comp_event_box_leave_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_mid_comp_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_mid_comp_event_box_leave_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_high_comp_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_high_comp_event_box_leave_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

GtkWidget*
make_meter (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

GtkWidget*
make_mscale (gchar *widget_name, gchar *string1, gchar *string2,
		gint int1, gint int2);

void
on_autoutton1_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_autoutton2_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_autoutton3_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_button11_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button12_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_undo_button_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_lim_lh_scale_value_changed          (GtkRange        *range,
                                        gpointer         user_data);

void
on_release_val_label_realize           (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_low2mid_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_low2mid_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_mid2high_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_mid2high_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_lim_input_hscale_value_changed      (GtkRange        *range,
                                        gpointer         user_data);

void
on_lim_input_hscale_realize            (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_notebook1_realize                   (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_optionmenu1_realize                 (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_low_meter_lbl_realize               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_mid_meter_lbl_realize               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_high_meter_lbl_realize              (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_low_meter_lbl_realize               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_mid_meter_lbl_realize               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_high_meter_lbl_realize              (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_high_meter_lbl_realize              (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_low_meter_lbl_realize               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_mid_meter_lbl_realize               (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
rev_button                             (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
fwd_button                             (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
play_toggle                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
stop_toggle                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
stop_toggle                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
rewind_button                          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
play_toggle                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
stop_toggle                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
rewind_transport                       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_autobutton_1_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_autobutton_2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_autobutton_3_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
play_toggle                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
stop_toggle                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
foward_transport                       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
forward_transport                      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_boost_scale_value_changed           (GtkRange        *range,
                                        gpointer         user_data);

gboolean
on_scene1_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene2_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene3_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene4_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene5_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene6_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene7_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene8_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene9_eventbox_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene10_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene11_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene12_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene13_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene14_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene15_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene16_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene17_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene18_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene19_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_scene20_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_setscene_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clearscene_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_help_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_EQ_curve_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_show_help                           (GtkWidget       *widget,
                                        GtkWidgetHelpType  help_type,
                                        gpointer         user_data);

gboolean
on_input_eventbox_enter_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_geq_eventbox_enter_notify_event     (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_spectrum_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_crossover_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_comp_curve_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_limiter_eventbox_enter_notify_event (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_boost_eventbox_enter_notify_event   (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_output_eventbox_enter_notify_event  (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_help_button_enter_notify_event      (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_eq_options_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_spectrum_options_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_transport_controls_eventbox_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_scenes_eventbox_enter_notify_event  (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_notebook1_switch_page               (GtkNotebook     *notebook,
                                        GtkWidget *page,
                                        guint            page_num,
                                        gpointer         user_data);

gboolean
on_window1_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_undo1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_redo1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_cc_window_delete_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_hscale_1_l_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
on_hscale_1_l_realize                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_hscale_1_m_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
on_hscale_1_m_realize                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_hscale_1_h_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
on_hscale_1_h_realize                  (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_frame_l_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_frame_m_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_frame_h_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_help1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_keys1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_ports1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_in1_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_left1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_right1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_out1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_left2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_right2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_ports1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_in1_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_left1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_right1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_out1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_left2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_right2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_out_trim_scale_value_changed        (GtkRange        *range,
                                        gpointer         user_data);


void
on_jack_ports_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about_prerequisites1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
scene_warning                          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
backward_transport                     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
play_transport                         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
stop_transport                         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
forward_transport                      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
toggle_transport_pause                 (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
pause_transport_toggle                 (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_event_box_enter_notify_event (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_stereo_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
reset_range                            (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_at_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_re_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_th_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_ra_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_kn_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_ma_label_1_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_at_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_re_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_th_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_ra_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_kn_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_ma_label_2_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_at_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_re_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_th_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_ra_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_kn_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_comp_ma_label_3_event_box_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_options1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_text_focus_in_event                 (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_text_focus_out_event                (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_low_active_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_low_mute_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_mid_active_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_mid_mute_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_high_active_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_high_mute_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_hscale_1_l_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
on_hscale_1_l_realize                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_hscale_1_m_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
on_hscale_1_m_realize                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_hscale_1_h_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
on_hscale_1_h_realize                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_low_bypass_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_mid_bypass_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_high_bypass_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_solo_toggled                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_eq_bypass_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_eq_bypass_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_band_button_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_limiter_bypass_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_limiter_bypass_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_name_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_scene_name_entry_changed            (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_scene_name_cancel_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_scene_name_ok_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_ft_bias_a_value_changed             (GtkRange        *range,
                                        gpointer         user_data);

void
on_ft_bias_b_value_changed             (GtkRange        *range,
                                        gpointer         user_data);

void
on_ft_rez_lp_a_value_changed           (GtkRange        *range,
                                        gpointer         user_data);

void
on_ft_rez_hp_a_value_changed           (GtkRange        *range,
                                        gpointer         user_data);

void
on_ft_rez_lp_b_value_changed           (GtkRange        *range,
                                        gpointer         user_data);

void
on_ft_rez_hp_b_value_changed           (GtkRange        *range,
                                        gpointer         user_data);

void
on_ft_bias_a_hp_value_changed          (GtkRange        *range,
                                        gpointer         user_data);

void
on_ft_bias_b_hp_value_changed          (GtkRange        *range,
                                        gpointer         user_data);

void
on_about_closebutton_clicked           (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_text_focus_in_event                 (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_text_focus_out_event                (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_MinGainSpin_value_changed           (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_MaxGainSpin_value_changed           (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_CrossfadeTimeSpin_value_changed     (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);


void
on_UpdateFrequencySpin_value_changed   (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_pref_close_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_pref_enter_notify_event             (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_FFTButton_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_IIRButton_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_inmeter_eventbox_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_outmeter_eventbox_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_lim_in_meter_eventbox_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_lim_out_meter_eventbox_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_warningLevelSpinButton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);


void 
callbacks_set_comp_bypass_button_state (int band, int state);


void 
callbacks_set_eq_bypass_button_state (int state);


void 
callbacks_set_limiter_bypass_button_state (int state);

void 
callbacks_set_low_delay_button_state (int state);

void 
callbacks_set_mid_delay_button_state (int state);

void
on_eqb_value_changed                   (GtkRange        *range,
                                        gpointer         user_data);

gboolean
on_eqb_enter_notify_event              (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_eqbl_enter_notify_event             (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_global_bypass_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void 
callbacks_blink_bypass_button (int button, int start);


gboolean
on_global_bypass_event_box_enter_notify_event
                                        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_meter_text_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_out_meter_peak_button_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_out_meter_full_button_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_rms_meter_peak_button_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_rms_meter_full_button_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_rmsTimeValue_value_changed          (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_hdeq_spectrum_color_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_reset_hdeq_curve1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_release_parametric_eq_controls1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cancel2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_help2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_scene_menu_help_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pref_help_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_limiter_combo_changed               (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_SpectrumComboBox_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_ColorsComboBox_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_LowDelayButton_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_MidDelayButton_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_LowDelaySpinButton_value_changed    (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_MidDelaySpinButton_value_changed    (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_logscale_scale_value_changed        (GtkRange        *range,
                                        gpointer         user_data);

gboolean
on_pref_dialog_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_pref_dialog_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_window2_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_show_help                           (GtkWidget       *widget,
                                        GtkWidgetHelpType  help_type,
                                        gpointer         user_data);

gboolean
on_window3_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);


gboolean
on_eButton1_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);
                                        
gboolean
on_eButton2_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);
                                        
void
on_presets_in_trim_scale_value_changed (GtkRange        *range,
                                        gpointer         user_data);

void
on_presets_pan_scale_value_changed     (GtkRange        *range,
                                        gpointer         user_data);

void
on_presets_out_trim_scale_value_changed
                                        (GtkRange        *range,
                                        gpointer         user_data);

gboolean
on_window4_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

