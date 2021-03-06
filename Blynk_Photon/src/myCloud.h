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
void assign_publist(Publish* pubList, const unsigned long now, const String unit, const String hm_string, const double control_time,
const double T);
void print_serial_header(void);
void serial_print(unsigned long now, double T);
String time_long_2_str(const unsigned long current_time, char *tempStr);
float prbs(uint8_t *seed);
void create_print_string(Publish *pubList);
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip);
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip);

#endif
