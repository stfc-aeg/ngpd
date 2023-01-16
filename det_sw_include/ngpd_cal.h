
#ifndef _NGPD_CAL_H
#define _NGPD_CAL_H
char * ngpd_cal_get_error_message();
int ngpd_cal_setup_all_ana(int path,  int card, int sync_boxes, int src);
int ngpd_cal_offsets(int path,  void (*nprintf)(void *msg_data, char *, ...), void *msg_data, int first_chan, int last_chan, double target, int num_pass, int adjust_only, char *save_fname);



#endif /* _NGPD_CAL_H */
