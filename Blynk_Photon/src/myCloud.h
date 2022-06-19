#ifndef my_cloud_h
#define my_cloud_h

// Publishing
struct Publish
{
  uint32_t now;
  String unit;
  String hm_string;
  double control_time;
  float Vbatt;
  float Tbatt;
  float Vshunt;
  float Ibatt;
  float Wbatt;
  float T;
  float Voc;
  float Voc_filt;
  float Vsat;
  boolean sat;
  float Tbatt_filt;
  int num_timeouts;
  float tcharge;
  float Amp_hrs_remaining_ekf;
  float Amp_hrs_remaining_wt;
  float soc_model;
  float soc;
  float soc_ekf;
  float soc_wt;
  float Vdyn;
  float Voc_ekf;
  float y_ekf;
};

void publish1(void);
void publish2(void);
void publish3(void);
void publish4(void);

#endif
