#define OFFSET_BITS 9  // VPO and PPO
#define PPN_BITS 11   // using 20 bit physical addresses
#define VPN_BITS 9   // using 18 bit virtual addresses

//   For the logging system
#define NEW_ADDRESS 0
#define ADDRESS_FROM_TLB 1
#define ADDRESS_FROM_PAGETABLE 2
#define DATA_FROM_CACHE 3
#define DATA_FROM_MEMORY 4
#define ILLEGALVIRTUAL 5
#define PHYSICALERROR 6
#define ILLEGALVPN 7
#define ADDRESS_FROM_PAGE_FAULT_HANDLER 8

void initialize();
int get_physical_address(int);
char get_byte(int);

extern int load_frame(int);
extern int get_word(int);
void  start_logging(), stop_logging(), log_entry(int,int);
