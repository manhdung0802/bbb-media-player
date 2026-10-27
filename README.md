# bbb-media-player
Plug USB to automaticaly create a playlist and play video on BBB. Control media player by hardware

# Output
Cắm USB + tự tạo playlist + phát video + điều khiển từ CANoe (chưa có CAN thì test với phím bấm)

# USB subsystem:
- là bus tự phát hiện - discoverable bus, khác hoàn toàn non-discoverable như device i2c hay spi
- khi cắm usb vào thì usb tự gửi vendor ID và Product ID tới kernel
- usb giao tiếp bằng cách đóng gói data vào khối URB (usb request). Mọi lệnh đọc ghi cần gói vào URB rồi đưa cho usb core để xếp lịch
- Interface:
    + là chức năng của usb device (chuột, phím, usb, ...)
    + 1 device cắm vào cổng usb có thể có nhiều chức năng
    + mỗi driver chỉ nên xử lý 1 chức năng, vì vậy nếu có nhiều chức năng thì mỗi chức năng cần có 1 driver
    + `bNumInterfaces` tổng số interface
- Endpoint:
    + Là các bộ đệm FIFO phần cứng nằm trên chip USB của thiết bị để lưu tạm dữ liệu gửi/nhận.
    + Mang tính đơn hướng: IN (thiết bị lên Host) hoặc OUT (Host xuống thiết bị).
    + Có 4 loại: Control (Endpoint 0), Interrupt, Bulk (dùng cho USB Flash/truyền file), Isochronous (âm thanh/video).
    + Tại sao cần: Cho phép truyền nhiều luồng dữ liệu độc lập (đọc, ghi, điều khiển) cùng lúc trên 1 cặp dây D+/D- duy nhất.
    + Không có Endpoint: Host không có địa chỉ nhận/gửi dữ liệu, thiết bị không thể nhận diện (liệt ngay khi cắm).
    + Endpoint của 1 usb do nhà sản xuất quyết định
    + muốn đọc hoặc ghi data vào usb, ta cần tạo đường ống với địa chỉ của endpoint để giao tiếp với usb
    + Endpoint chỉ có 1 chiều, nên nếu cần nhiều chiều giao tiếp thì cần đọc nhiều Endpoint
- URB - USB request block
    + là những gói tin truyền trong đường ống Endpoint
    + Sau khi được đóng gói, các URB này được USB core xếp lịch để truyền tải
    + Driver chỉ việc đóng gói URB và gửi đi, kernel sẽ đưa URB vào hàng đợi để xử lý bất đồng bộ
    + Khi truyền nhận URB xong, USB core sẽ báo lại cho driver
    + trước khi gửi hay nhận 1 URB, cần tạo phiếu URB bằng `usb_fill_bulk_urb`
    + 1 URB có thể đọc ghi 1 số byte lớn hơn `wMaxPacketSize`, nhưng usb controller sẽ băm nhỏ thành nhiều gói tin có kích thước `wMaxPacketSize`

# Các bước trong Probe
- cấp phát struct quản lý usb
- duyệt qua các interface, endpoint, lấy các giá trị của địa chỉ endpoint, bulk_in, bulk_out
- cấp phát bộ nhớ đệm để chứa data, kích thước bộ đệm phải >= `wMaxPacketSize`
- tạo device file để userspace tương tác: `usb_register_dev`