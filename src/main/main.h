typedef struct key_data_t{
  int modifier;
  int value;
}key_data_t;

typedef struct key_status_t{
  //The current effect the key has on the modifiers
  int modifier;
  //The index in the reported usb_keys if it has been been previously assigned
  //UNASSIGNED_USB_KEY otherwise.
  int usb_key;
  int state;
  int key_nr; //The order this key was when pressed
  int value;
} key_status_t;

const int MAX_LEN = 48;

void init_main();
unsigned int key_addr(int x, int y, int l);
void run_main();

