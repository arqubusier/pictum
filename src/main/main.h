
typedef struct _key_elem_t{
  //val == LOW means the key is pressed
  //val == HIGH means the key is pressed
  int val;
  int modifier;
  int normal_key;
  int usb_key;
}key_elem_t;


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
key_elem_t *get_key_left(int row_pin, int col_pin, int fn_l);
int set_key(key_elem_t *k, int val);
void reset_usb();
void set_usb(key_elem_t *k, int usb_key);
void run_main();
