typedef struct key_val_t{
  int hid_code;
  int usb_key;
}key_val_t;


class int_stack{
private:
  
  static const int MAX_LEN = 6;
  int head;
  const int EMPTY = -1;
  int arr[MAX_LEN];

public:
  
  void push(int val){
    if (head < MAX_LEN){
      ++head;
      arr[head] = val;
    }
  }
  
  int pop(){
    if (head == EMPTY)
      return EMPTY;
    
    int val = arr[head];
    --head;
    return val;
  }
};

void init_main();
unsigned int key_addr(int x, int y, int l);
key_val_t *get_key(int row_pin, int col_pin, int fn_l);
int set_key(key_elem_t *k, int val);
void reset_usb();
void set_usb(key_val_t *k, int usb_key);
void set_row_left(int row);
void set_row_right(int row);
void get_row_left(int *row_buff);
void get_row_right(int *row_buff);
bool get_key_status(int row, int col);
key_val_t *get_key(int row, int col);
void run_main();
