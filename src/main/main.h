typedef struct _key_elem_t{
  //val == LOW means the key is pressed
  //val == HIGH means the key is pressed
  int val;
  int modifier;
  int normal_key;
  struct _key_elem *next;
  struct _key_elem *prev;
}key_elem_t;

void init_main();
unsigned int key_addr(int x, int y, int l);
key_elem_t get_key_left(int row_pin, int col_pin, int fn_l);
void set_key_val_left(int row_pin, int col_pin, int fn_l, int val);
void reset_usb_buff();
void set_usb(key_elem_t k);
void run_main();
