/*
 * ============================================================================
 * GIẢI THÍCH CHI TIẾT TỪNG TRƯỜNG TRONG struct usb_driver (THEO ĐÚNG THỨ TỰ)
 * ============================================================================
 *
 * 1. const char *name:
 *    - Khái niệm: Tên chuỗi (human-readable string) định danh driver trong
 *      Linux Device Model (ví dụ: "bbb-usb", "usbhid", "usb-storage").
 *    - Tại sao Kernel cần: Xuất hiện trong log kernel (dmesg), tạo thư mục
 *      tại /sys/bus/usb/drivers/<name>, và giúp udev / lsmod phân biệt driver.
 *    - Khi nào set / bỏ qua: BẮT BUỘC PHẢI SET. Tên phải là duy nhất,
 *      thường đặt trùng với tên module.
 *
 * 2. int (*probe) (struct usb_interface *intf, const struct usb_device_id *id):
 *    - Khái niệm: Con trỏ hàm khởi tạo thiết bị. Được usbcore gọi ngay khi
 *      phát hiện một interface USB khớp với một mục trong id_table.
 *    - Tại sao Kernel cần: Điểm khởi đầu của driver để: kiểm tra các Endpoint
 *      (Bulk, Interrupt, Isochronous), cấp phát bộ nhớ private data, cấu hình
 *      các gói truyền tin URB (USB Request Blocks), liên kết với các subsystem
 *      khác (Char dev, Input, ALSA...), và lưu private data thông qua
 *      hàm usb_set_intfdata(intf, priv).
 *    - Khi nào set / bỏ qua: BẮT BUỘC PHẢI SET. Không có hàm này thì driver
 *      không thể nhận và điều khiển phần cứng.
 *
 * 3. void (*disconnect) (struct usb_interface *intf):
 *    - Khái niệm: Con trỏ hàm thu dọn tài nguyên. Được gọi khi thiết bị USB
 *      bị rút ra khỏi cổng cắm (hot-unplug) hoặc khi module bị gỡ (rmmod).
 *    - Tại sao Kernel cần: Thiết bị USB có thể bị rút đột ngột bất kỳ lúc nào.
 *      Kernel cần hàm này để driver hủy toàn bộ các URB đang chờ (usb_kill_urb),
 *      giải phóng bộ nhớ, hủy đăng ký file thiết bị trong /dev, và dọn dẹp
 *      usb_set_intfdata(intf, NULL).
 *    - Khi nào set / bỏ qua: BẮT BUỘC PHẢI SET. Bỏ qua hàm này sẽ gây rò rỉ
 *      bộ nhớ (memory leak), kẹt luồng I/O và sập kernel (Kernel Panic) ngay
 *      khi rút USB.
 *
 * 4. int (*unlocked_ioctl) (struct usb_interface *intf, unsigned int code, void *buf):
 *    - Khái niệm: Callback nhận các mã lệnh IOCTL đặc biệt trực tiếp từ userspace
 *      thông qua file node usbfs (/dev/bus/usb/...).
 *    - Tại sao Kernel cần: Cho phép ứng dụng userspace can thiệp sâu hoặc gửi
 *      lệnh IOCTL tùy biến riêng xuống driver mà không cần qua character device.
 *    - Khi nào set / bỏ qua: HẦU NHƯ LUÔN BỎ QUA, trừ khi viết driver có giao thức
 *      riêng với userspace qua usbfs.
 *
 * 5. int (*suspend) (struct usb_interface *intf, pm_message_t message):
 *    - Khái niệm: Callback tạm dừng hoạt động của interface trước khi thiết bị
 *      hoặc hệ thống chuyển sang chế độ ngủ (Sleep/Suspend/Autosuspend)
 *      để tiết kiệm điện.
 *    - Tại sao Kernel cần: Yêu cầu driver dừng gửi nhận dữ liệu, tạm ngưng các
 *      URB đang chạy và đưa chip phần cứng vào trạng thái tiêu thụ điện năng thấp.
 *    - Khi nào set / bỏ qua: CÓ THỂ BỎ QUA với driver đơn giản/học tập.
 *      NÊN SET cho các sản phẩm thương mại hoặc thiết bị chạy pin.
 *
 * 6. int (*resume) (struct usb_interface *intf):
 *    - Khái niệm: Callback đánh thức interface khi hệ thống thức dậy sau chế độ ngủ.
 *    - Tại sao Kernel cần: Cho phép driver khôi phục lại trạng thái làm việc
 *      bình thường, submit lại các URB nhận dữ liệu.
 *    - Khi nào set / bỏ qua: CÓ THỂ BỎ QUA nếu không dùng suspend. Nếu đã viết
 *      suspend thì BẮT BUỘC phải có resume.
 *
 * 7. int (*reset_resume) (struct usb_interface *intf):
 *    - Khái niệm: Callback xử lý khi thiết bị thức dậy nhưng trong lúc ngủ đã bị
 *      mất nguồn hoàn toàn (power lost) và vừa bị reset lại từ đầu.
 *    - Tại sao Kernel cần: Khi mất nguồn, mọi thanh ghi cấu hình bên trong chip
 *      USB bị xóa về mặc định. Kernel gọi hàm này để driver nạp lại firmware
 *      hoặc cấu hình lại thanh ghi từ đầu mà không cần gọi lại disconnect rồi probe.
 *    - Khi nào set / bỏ qua: CÓ THỂ BỎ QUA trừ khi thiết bị cần nạp lại cấu hình
 *      đặc biệt sau khi mất nguồn.
 *
 * 8. int (*pre_reset) (struct usb_interface *intf):
 *    - Khái niệm: Callback được gọi trước khi usbcore thực hiện reset lại cổng USB
 *      (usb_reset_device) do lỗi đường truyền hoặc bus recovery.
 *    - Tại sao Kernel cần: Báo driver tạm ngưng gửi URB mới, dọn dẹp các gói tin
 *      dở dang trước khi bus bị dập xung reset vật lý.
 *    - Khi nào set / bỏ qua: BỎ QUA với hầu hết driver thông thường. Thường dùng
 *      trong USB Mass Storage để tránh hỏng dữ liệu filesystem.
 *
 * 9. int (*post_reset) (struct usb_interface *intf):
 *    - Khái niệm: Callback được gọi sau khi usbcore hoàn thành việc reset cổng USB.
 *    - Tại sao Kernel cần: Cho phép driver cấu hình lại interface, khôi phục lại
 *      trạng thái các endpoint và tiếp tục gửi/nhận URB.
 *    - Khi nào set / bỏ qua: BỎ QUA nếu không dùng pre_reset.
 *
 * 10. const struct usb_device_id *id_table:
 *     - Khái niệm: Bảng danh sách các thiết bị phần cứng mà driver này hỗ trợ
 *       (danh sách lọc theo Vendor ID, Product ID, hoặc Interface Class),
 *       kết thúc bằng một phần tử rỗng {}.
 *     - Tại sao Kernel cần: USB là bus cắm rút nóng (Hotplug & Discoverable).
 *       Khi cắm thiết bị, usbcore đọc descriptor từ chip rồi so sánh với
 *       id_table của các driver đã đăng ký để quyết định gọi hàm probe() nào.
 *       Macro MODULE_DEVICE_TABLE(usb, id_table) cũng trích xuất bảng này
 *       để udev tự động gọi modprobe nạp module khi cắm USB.
 *     - Khi nào set / bỏ qua: BẮT BUỘC PHẢI SET. Nếu để NULL, hàm probe()
 *       của driver sẽ không bao giờ được gọi khi cắm thiết bị.
 *
 * 11. const struct attribute_group **dev_groups:
 *     - Khái niệm: Mảng các nhóm thuộc tính Sysfs (file đọc/ghi) thuộc về interface.
 *     - Tại sao Kernel cần: Kernel tự động tạo các file trong
 *       /sys/bus/usb/devices/.../ khi driver bind thành công và xóa đi khi unbind,
 *       không cần gọi thủ công sysfs_create_group().
 *     - Khi nào set / bỏ qua: BỎ QUA nếu driver không cần expose các file cấu hình
 *       ra sysfs cho userspace đọc ghi.
 *
 * 12. struct usb_dynids dynids:
 *     - Khái niệm: Danh sách liên kết nội bộ lưu các ID thiết bị được thêm động
 *       tại runtime thông qua sysfs (file new_id).
 *     - Tại sao Kernel cần: Cho phép quản lý và duyệt qua các ID động do người dùng
 *       thêm vào mà không cần biên dịch lại driver.
 *     - Khi nào set / bỏ qua: TUYỆT ĐỐI BỎ QUA. Do usbcore tự động quản lý.
 *
 * 13. struct usbdrv_wrap drvwrap:
 *     - Khái niệm: Cấu trúc bọc nội bộ chứa struct device_driver của Linux
 *       Driver Model tổng.
 *     - Tại sao Kernel cần: Kết nối struct usb_driver vào cây phả hệ driver
 *       chung của toàn bộ nhân Linux.
 *     - Khi nào set / bỏ qua: TUYỆT ĐỐI BỎ QUA. Kernel tự động điền khi gọi
 *       usb_register() hoặc module_usb_driver().
 *
 * 14. unsigned int no_dynamic_id:1:
 *     - Khái niệm: Cờ 1 bit cấm userspace ép gán thêm ID thiết bị mới tại runtime
 *       qua sysfs (/sys/bus/usb/drivers/<name>/new_id).
 *     - Tại sao Kernel cần: Ngăn người dùng ép driver nhận diện sai thiết bị phần
 *       cứng, tránh nguy cơ gây lỗi hỏng hóc hoặc treo hệ thống.
 *     - Khi nào set / bỏ qua: BỎ QUA (= 0) để mặc định cho phép thêm dynamic id khi debug.
 *
 * 15. unsigned int supports_autosuspend:1:
 *     - Khái niệm: Cờ 1 bit xác nhận driver này hỗ trợ tự động ngủ tiết kiệm
 *       điện (Runtime Autosuspend) khi thiết bị rảnh rỗi.
 *     - Tại sao Kernel cần: usbcore chỉ cho phép cổng USB tự động ngủ nếu TẤT CẢ
 *       các driver đang quản lý các interface của thiết bị đều bật cờ này (= 1).
 *     - Khi nào set / bỏ qua: BỎ QUA (= 0) với driver cơ bản. Set = 1 nếu driver
 *       đã hỗ trợ đầy đủ suspend/resume.
 *
 * 16. unsigned int disable_hub_initiated_lpm:1:
 *     - Khái niệm: Cờ vô hiệu hóa tính năng quản lý nguồn tầng liên kết (Link Power
 *       Management - LPM trên USB 3.0) do Hub khởi xướng.
 *     - Tại sao Kernel cần: Một số chip USB 3.0 bị lỗi phần cứng sẽ bị rớt gói tin
 *       nếu Hub tự ý chuyển link sang U1/U2 lúc rảnh rỗi.
 *     - Khi nào set / bỏ qua: BỎ QUA, chỉ set = 1 nếu phần cứng USB 3.0 bị lỗi LPM.
 *
 * 17. unsigned int soft_unbind:1:
 *     - Khái niệm: Cờ báo cho usbcore không tự động hủy (kill) các URB trước khi
 *       gọi hàm disconnect().
 *     - Tại sao Kernel cần: Mặc định usbcore dọn sạch URB trước khi disconnect.
 *       Nếu driver muốn tự gửi gói tin chào tạm biệt (goodbye packet) xuống
 *       thiết bị trong disconnect, nó cần bật cờ này.
 *     - Khi nào set / bỏ qua: HẦU NHƯ LUÔN BỎ QUA (= 0).
 *
 * ============================================================================
 * TÓM TẮT: VỚI MỘT USB DRIVER CƠ BẢN, BẠN CHỈ CẦN SET ĐÚNG 4 TRƯỜNG:
 * 1. .name        (Tên driver)
 * 2. .probe       (Hàm khởi tạo khi cắm USB)
 * 3. .disconnect  (Hàm thu dọn khi rút USB)
 * 4. .id_table    (Bảng lọc phần cứng VID/PID)
 * ============================================================================
 */

#define pr_fmt(fmt) "%s " fmt "\n", __func__
#include "linux/device.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>

int bbb_probe(struct usb_interface *intf,
	      const struct usb_device_id *id);
void bbb_disconnect(struct usb_interface *intf);
ssize_t bbb_read(struct file *, char __user *, size_t, loff_t *);
ssize_t bbb_write(struct file *, const char __user *, size_t, loff_t *);
int bbb_open(struct inode *, struct file *);
int bbb_release(struct inode *, struct file *);

static const struct usb_device_id bbb_table[] = {
    {USB_DEVICE(0x0781, 0x5597)},
    {},
};
MODULE_DEVICE_TABLE(usb, bbb_table);

static struct usb_driver bbb_usb_driver = {
    .name = "bbb-usb",
    .probe = bbb_probe,
    .disconnect = bbb_disconnect,
    .id_table = bbb_table,
};
module_usb_driver(bbb_usb_driver);

struct my_bbb_usb_driver {
	struct usb_interface *intf;
	struct usb_endpoint_descriptor *bulk_in;
	struct usb_endpoint_descriptor *bulk_out;
	unsigned int pipe_in;
	unsigned int pipe_out;
	unsigned int endpoint_in;
	unsigned int endpoint_out;
	struct usb_device *udev;
	void *bulk_in_buffer;
	void *bulk_out_buffer;
};

const struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = bbb_write,
    .read = bbb_read,
    .open = bbb_open,
    .release = bbb_release,
};

// minor_base: bắt buộc set, nên set từ 192 trở đi
struct usb_class_driver class_driver = {
    .name = "bbb_media_player%d",
    .fops = &fops,
    .minor_base = 192,
};

int bbb_probe(struct usb_interface *intf,
	      const struct usb_device_id *id) {
	pr_info("");
	int ret;
	struct my_bbb_usb_driver *my_bbb_usb_driver = devm_kzalloc(&intf->dev, sizeof(struct my_bbb_usb_driver), GFP_KERNEL);
	struct usb_host_interface *cur_altsetting = intf->cur_altsetting;

	usb_set_intfdata(intf, my_bbb_usb_driver);
	my_bbb_usb_driver->intf = intf;

	// Kiểm tra interface có khớp interface mình cần không
	if (cur_altsetting->desc.bInterfaceClass != USB_CLASS_MASS_STORAGE) {
		pr_info("Device is not a storage");
		return -ENODEV;
	}
	pr_info("Found a mass storage");

	// Khởi tạo các thông tin cho gói tin URB
	// bulk in - usb to BBB
	ret = usb_find_bulk_in_endpoint(cur_altsetting, &my_bbb_usb_driver->bulk_in);
	if (ret != 0) {
		pr_info("Cannot find a bulk_in");
		return ret;
	}
	my_bbb_usb_driver->endpoint_in = my_bbb_usb_driver->bulk_in->bEndpointAddress;
	my_bbb_usb_driver->udev = interface_to_usbdev(intf);
	my_bbb_usb_driver->pipe_in = usb_rcvbulkpipe(my_bbb_usb_driver->udev, my_bbb_usb_driver->endpoint_in);

	// bulk out - BBB to usb
	ret = usb_find_bulk_out_endpoint(cur_altsetting, &my_bbb_usb_driver->bulk_out);
	if (ret != 0) {
		pr_info("Cannot find a bulk_out");
		return ret;
	}
	my_bbb_usb_driver->endpoint_out = my_bbb_usb_driver->bulk_out->bEndpointAddress;
	my_bbb_usb_driver->udev = interface_to_usbdev(intf);
	my_bbb_usb_driver->pipe_out = usb_sndbulkpipe(my_bbb_usb_driver->udev, my_bbb_usb_driver->endpoint_out);

	// Cấp phát buffer >= wMaxPacketSize để chứa được data
	__le16 wMaxPacketSize_in = usb_endpoint_maxp(my_bbb_usb_driver->bulk_in);
	my_bbb_usb_driver->bulk_in_buffer = devm_kzalloc(&intf->dev, wMaxPacketSize_in, GFP_KERNEL);
	__le16 wMaxPacketSize_out = usb_endpoint_maxp(my_bbb_usb_driver->bulk_out);
	my_bbb_usb_driver->bulk_out_buffer = devm_kzalloc(&intf->dev, wMaxPacketSize_out, GFP_KERNEL);

	// tạo device file để userspace tương tác
	usb_register_dev(intf, &class_driver);

	return 0;
}

void bbb_disconnect(struct usb_interface *intf) {
	pr_info("");
	usb_deregister_dev(intf, &class_driver);
}

ssize_t bbb_read(struct file *file, char __user *buf, size_t count, loff_t *off) {
	pr_info("");
	return count;
}
ssize_t bbb_write(struct file *file, const char __user *buf, size_t count, loff_t *off) {
	pr_info("");
	return count;
}
int bbb_open(struct inode *inode, struct file *file) {
	pr_info("");
	return 0;
}
int bbb_release(struct inode *inode, struct file *file) {
	pr_info("");
	return 0;
}

MODULE_LICENSE("GPL");