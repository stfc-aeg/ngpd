/*****************************************************************************
*
* Header for XH demo driver statics
*
* by William Helsby
*
*****************************************************************************/


#define HAVE_EXTRA_IOTCL			1		// Pass unknown ioctl to zynqmp_extra_ioctl

#define HIST_CACHE_ALL_COHERENT		0		// Hist Frames and List are on ACP an Coherent
#define HIST_CACHE_LIST_COHERENT		1		// Hist List is on ACP and Coherent, Frames on AXI_HP and not coherent
#define HIST_CACHE_JUST_SOFT_BUFFER		2		// None is Coherent, just cache the software buffer.


#define HIST_CACHE_OPTION HIST_CACHE_LIST_COHERENT
#define ACCESS_FORCE32BIT 1
#define ACCESS_CACHEFLUSH 2

typedef struct _dev_static
{
    struct device *dev;
    struct cdev cdev;
	dev_t dev_num;
    struct class *class;
	int cdev_done;
	int valid;
	int usecount;
	u_int64_t phys_base[ZYNQMP_NUM_ADDR_SPACE];
	u_int64_t size_bytes[ZYNQMP_NUM_ADDR_SPACE];
	int access_flags[ZYNQMP_NUM_ADDR_SPACE];
	u_int32_t __iomem *virt_base[ZYNQMP_NUM_ADDR_SPACE];
	int hist_list_irq, hist_frame_irq, scalar_irq;
	struct semaphore sem;
	char addr_space_name[ZYNQMP_NUM_ADDR_SPACE][20];
	ZYNQMP_DMA_StatusBlock status_block;
	DMAPrivate dma_priv[ZYNQMP_DMA_STREAM_NUM];
	ZYNQMP_DMA_MsgBuildDesc dma_last_build_desc[ZYNQMP_DMA_STREAM_NUM];
	ZYNQMP_DMA_MsgBuildDebugDesc dma_last_build_debug_desc[ZYNQMP_DMA_STREAM_NUM];
	enum { NGPDZynqMP_XU7 } zynqmp_version;
	struct tasklet_struct hist_tasklet;
	struct rw_semaphore dma_sem;
//	struct mutex dma_mutex;
	int hist_errors;
#if HIST_CACHE_OPTION!=HIST_CACHE_ALL_COHERENT
	u_int32_t * hist_virt_base;
	u_int32_t hist_phys_base;
#endif
	int num_scope;
	int last_checked[ZYNQMP_DMA_STREAM_NUM];
	u_int64_t num_good[ZYNQMP_DMA_STREAM_NUM];
	u_int64_t num_completed[ZYNQMP_DMA_STREAM_NUM];
	u_int64_t next_time_frame[ZYNQMP_DMA_STREAM_NUM];
	u_int64_t last_time_frame[ZYNQMP_DMA_STREAM_NUM];
	struct mutex hist_list_mutex;
	struct mutex hist_frame_mutex;
	struct mutex scalar_mutex;
	struct socket *hist_list_server_sock;
	struct socket *hist_frame_server_sock;
	struct socket *scalar_server_sock;
	struct socket *hist_list_transfer_sock;
	struct socket *hist_frame_transfer_sock;
	struct socket *scalar_transfer_sock;
	void (*orig_data_ready)(struct sock *sk);
	void (*orig_state_change)(struct sock *sk);
	struct work_struct hist_list_accept;
	struct work_struct hist_frame_accept;
	struct work_struct scalar_accept;
	struct work_struct hist_list_send;
	struct work_struct hist_frame_send;
	struct work_struct scalar_send;

	struct work_struct hist_list_close_wait;
	struct work_struct hist_frame_close_wait;
	struct work_struct scalar_close_wait;
	int readout_mode;
	int eof_count;

} DevStatics;


extern DevStatics *xsp4_dev_statics;

extern int debug;
extern char * driver_name;
extern int num_devices;
extern struct workqueue_struct * xsp4_wq;

int zynqmp_open (struct inode *inode, struct file *filp);
int zynqmp_close (struct inode *inode, struct file *filp);
int zynqmp_mmap (struct file *file, struct vm_area_struct *vma);
int zynqmp_dma_ioctl(DevStatics *dev_stat, ZynqMPPb *param);
int zynqmp_dma_check_stream(DevStatics *dev_stat, int stream);
int zynqmp_dma_config_memory(DevStatics *dev_stat, int layout);

long zynqmp_ioctl ( struct file *filp, unsigned int cmd, unsigned long arg) ;

irqreturn_t zynqmp_dma_irq_handler(int irq, void *data);

irqreturn_t zynqmp_hist_frame_dma_irq_handler(int irq, void *data);
irqreturn_t zynqmp_scalar_dma_irq_handler(int irq, void *data);

void hist_tasklet(unsigned long mnum);
void zynqmp_frame_worker_scalar(struct work_struct *work);
void zynqmp_frame_worker_hist(struct work_struct *work);
void zynqmp_hist_list_worker(struct work_struct *work);

int zynqmp_socket_send(struct socket *sock, unsigned char * data, int len);
u_int32_t * zynqmp_dma_buffer_ptr(DevStatics *dev_stat, int stream, u_int64_t byte_offset, u_int64_t *remaining_bytes, u_int64_t *chunk_bytes, int *addr_space, u_int64_t *dma_base, int *seg_num);

#define ZYNQMP_IOWrite32(addr,value)  writel(value,addr)
#define ZYNQMP_IORead32(addr)  readl(addr)
#define ZYNQMP_IOWrite64(addr,value)  writeq(value,addr)
#define ZYNQMP_IORead64(addr)  readq(addr)

long zynqmp_extra_ioctl ( unsigned int cmd, DevStatics *dev_stat, ZynqMPPb *param);
