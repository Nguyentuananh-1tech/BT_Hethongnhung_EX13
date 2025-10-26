
## **Bài 1 **

### **Yêu cầu**

Viết chương trình có 2 **Task**:

1. **Task 1 – Blink LED1:** LED1 (PB5) nhấp nháy liên tục với chu kỳ 1 giây.
2. **Task 2 – Xử lý ngắt (LED2):** Khi nhấn nút (ngắt ngoài tại PA1), LED2 (PB11) sáng trong 1 giây, sau đó tắt.

Khi **chưa nhấn nút**, Task điều khiển LED2 sẽ **ở trạng thái Block**, chỉ được kích hoạt khi có ngắt ngoài xảy ra.
Khi **có ngắt ngoài**, LED2 phải được bật **ngay sau khi ISR thực thi xong**, thể hiện phản ứng tức thời của hệ thống.

---

### **Ý tưởng triển khai**

* Sử dụng **binary semaphore** để đồng bộ giữa **ngắt ngoài (ISR)** và **Task điều khiển LED2**.
* Khi xảy ra ngắt ngoài (do nhấn nút ở PA1), hàm xử lý ngắt (**EXTI1_IRQHandler**) **không trực tiếp bật LED**, mà **chỉ thực hiện cấp semaphore** (`xSemaphoreGiveFromISR`).
* Task LED2 luôn ở trạng thái **Block** bằng lệnh `xSemaphoreTake(..., portMAX_DELAY)` và chỉ **thức dậy** khi semaphore được cấp từ ISR.
* Sau khi thức dậy, Task LED2 bật đèn trong 1 giây, sau đó tắt và quay lại trạng thái chờ.
* Việc gọi `portYIELD_FROM_ISR()` trong hàm ngắt đảm bảo rằng **nếu Task LED2 có độ ưu tiên cao hơn**, CPU sẽ **chuyển ngữ cảnh ngay lập tức** để Task LED2 được chạy ngay sau khi ISR kết thúc.
* Trong khi đó, **Task LED1** hoạt động nền, cứ mỗi 1 giây nháy LED PB5 một lần, minh họa cho hoạt động song song trong FreeRTOS.

---

### **Các bước thực hiện**

1. **Cấu hình GPIO:**

   * LED1 tại **PB5** (Output Push-Pull, trạng thái ban đầu bật).
   * LED2 tại **PB11** (Output Push-Pull, trạng thái ban đầu tắt).
   * Nút nhấn tại **PA1** (Input Pull-Up).

2. **Cấu hình ngắt ngoài (EXTI1):**

   * Kích hoạt ngắt **Falling Edge** tại PA1.
   * Cài đặt NVIC cho kênh **EXTI1_IRQn** với mức ưu tiên thích hợp.

3. **Tạo semaphore nhị phân:**

   * Dùng `xSemaphoreCreateBinary()` để tạo semaphore dùng chung giữa ISR và Task LED2.

4. **Tạo hai Task:**

   * **Task_LED1:** Nháy LED1 liên tục (delay 1s).
   * **Task_LED2:** Chờ semaphore; khi nhận được, bật LED2 trong 1 giây rồi tắt.

5. **Hàm xử lý ngắt (EXTI1_IRQHandler):**

   * Khi có ngắt, xóa cờ ngắt bằng `EXTI_ClearITPendingBit()`.
   * Gửi semaphore đến Task_LED2 (`xSemaphoreGiveFromISR`).
   * Gọi `portYIELD_FROM_ISR()` để kích hoạt chuyển ngữ cảnh nếu cần.

---

### **Kết quả mong đợi**

* Khi hệ thống hoạt động bình thường (chưa nhấn nút):

  * LED1 (PB5) nhấp nháy đều 1 giây/lần.
  * LED2 (PB11) tắt và Task_LED2 ở trạng thái Block.

* Khi nhấn nút PA1:

  * Ngắt ngoài được kích hoạt.
  * ISR cấp semaphore cho Task_LED2.
  * Task_LED2 chạy ngay sau ISR, bật LED2 sáng 1 giây rồi tắt.
  * Sau đó, Task_LED2 trở lại trạng thái chờ cho lần ngắt tiếp theo.

* Quá trình này lặp lại ổn định, cho thấy sự **đồng bộ hiệu quả giữa ngắt ngoài và task** trong FreeRTOS thông qua **binary semaphore**.

---

---

## **Bài 2 **

### **Yêu cầu**

Viết chương trình có **2 Task** cùng truyền dữ liệu ra **cổng UART2**, **không sử dụng mutex hoặc bất kỳ cơ chế đồng bộ nào**, nhằm **thể hiện sự xung đột (race condition)** khi nhiều task cùng truy cập một tài nguyên dùng chung.

---

### **Ý tưởng triển khai**

* Chương trình sử dụng **FreeRTOS** với **2 task song song**:

  * **Task A:** Gửi chuỗi `"hello"` qua UART2.
  * **Task B:** Gửi chuỗi `"xinchao"` qua UART2.
* Cả hai task đều truy cập trực tiếp vào cùng một **hàm gửi UART2** mà **không có biện pháp bảo vệ tài nguyên chung**.
* Mỗi task lần lượt gửi từng ký tự của chuỗi, có thêm **`vTaskDelay(1ms)`** giữa mỗi ký tự, để tạo điều kiện dễ xảy ra **xen lẫn dữ liệu** giữa hai task.
* Sau khi gửi xong một chuỗi, mỗi task chờ **3 giây** rồi lặp lại.
* UART2 được cấu hình ở tốc độ **115200 baud**, dùng **PA2 (TX)** và **PA3 (RX)**.

---

### **Nguyên lý hoạt động**

* **Cả hai task** có **độ ưu tiên bằng nhau**, được lập lịch xen kẽ theo cơ chế **preemptive scheduling** của FreeRTOS.
* Vì **không có mutex** hay semaphore bảo vệ, hai task có thể **truy cập vào UART cùng lúc**.
* Khi một task đang gửi từng ký tự mà bị scheduler chuyển sang task kia, **dữ liệu của hai chuỗi có thể xen lẫn nhau**, tạo ra **kết quả lộn xộn** trên terminal UART.
* Việc thêm lệnh `vTaskDelay(1)` giữa mỗi ký tự càng tăng xác suất xảy ra xung đột, giúp quan sát rõ hơn hiện tượng.

---

### **Cấu trúc chương trình**

1. **Khởi tạo UART2:**

   * Cấu hình chân **PA2 (TX)**, **PA3 (RX)**.
   * Cấu hình tốc độ baud, stop bit, parity, mode truyền/nhận.
   * Kích hoạt USART2.

2. **Tạo hai Task:**

   * **Task A:** Gửi chuỗi `"hello"`.
   * **Task B:** Gửi chuỗi `"xinchao"`.
   * Mỗi task sử dụng hàm `uart2_send_char()` để gửi từng ký tự.

3. **Cấu hình FreeRTOS:**

   * Cả hai task được tạo với **cùng độ ưu tiên (tskIDLE_PRIORITY + 1)**.
   * Scheduler bắt đầu điều phối song song 2 task.

4. **Không có Mutex hoặc Semaphore bảo vệ UART**, nên tài nguyên này được truy cập **cạnh tranh (unsynchronized)**.

---

### **Kết quả mong đợi**

* Khi mở **terminal UART (115200 baud)**, ta sẽ thấy dữ liệu **bị trộn lẫn** giữa hai chuỗi `"hello"` và `"xinchao"`.
* Ví dụ, thay vì xuất lần lượt:

  ```
  hello
  xinchao
  ```

  ta có thể thấy:

  ```
  hexi nlchl aoo
  xinhello
  ```
* Kết quả thay đổi tùy theo cách scheduler phân chia CPU cho hai task — minh họa rõ hiện tượng **race condition** trong hệ thống đa nhiệm.

---

### **Kết luận**

Chương trình này thể hiện **vấn đề khi hai task cùng truy cập tài nguyên dùng chung mà không có cơ chế bảo vệ**.
Đây là cơ sở để sang **Bài 3**, nơi sẽ sử dụng **mutex** để loại bỏ xung đột và đảm bảo dữ liệu UART được in ra **nguyên vẹn, có thứ tự**.

---

---

## **Bài 3*

### **Mục tiêu**

Mục tiêu của bài này là **gửi chuỗi ký tự qua cổng UART** từ **hai tác vụ chạy song song trong FreeRTOS**, đảm bảo **tránh xung đột truy cập tài nguyên UART** bằng cách sử dụng **Semaphore kiểu Mutex** để đồng bộ hóa.

---

### **Ý tưởng thực hiện**

Chương trình tạo **hai tác vụ (TaskA và TaskB)**, mỗi tác vụ gửi một chuỗi ký tự khác nhau (“hello” và “xinchao”) qua cổng **USART2**.
Để tránh việc hai tác vụ cùng truy cập UART cùng lúc (gây lỗi dữ liệu), chương trình sử dụng **Semaphore Mutex (xUartMutex)** — chỉ cho phép **một tác vụ được gửi dữ liệu tại một thời điểm**.

---

### **Cấu trúc và hoạt động chính**

1. **Khởi tạo UART2**

   * Cấu hình các chân **PA2 (TX)** và **PA3 (RX)** ở chế độ tương ứng.
   * Thiết lập tốc độ truyền (baudrate) là **115200 bps**.
   * Kích hoạt **USART2** để truyền và nhận dữ liệu.

2. **Khởi tạo Semaphore**

   * Tạo một **Mutex** (xUartMutex) dùng để khóa tài nguyên UART khi đang gửi dữ liệu.
   * Khi một tác vụ cần gửi dữ liệu, nó sẽ **“take” (giữ)** Mutex; khi gửi xong, nó **“give” (trả lại)** để tác vụ khác có thể sử dụng.

3. **Tác vụ A (TaskA)**

   * Cứ mỗi **3 giây**, tác vụ lấy Mutex, gửi chuỗi `"hello"` qua UART, sau đó trả lại Mutex.
   * Sử dụng hàm `vTaskDelayUntil()` để đảm bảo chu kỳ truyền ổn định 3 giây/lần.

4. **Tác vụ B (TaskB)**

   * Hoạt động tương tự TaskA, nhưng gửi chuỗi `"xinchao"`.
   * Cũng tuân thủ chu kỳ **3 giây**, sử dụng cùng cơ chế Mutex để tránh xung đột UART.

---

### **Nguyên lý hoạt động của cơ chế đồng bộ**

* Khi **TaskA hoặc TaskB** muốn gửi dữ liệu, chúng phải **xSemaphoreTake(xUartMutex)**.
* Nếu Mutex đang được tác vụ khác giữ, tác vụ sẽ **chờ đến khi được trả lại (portMAX_DELAY)**.
* Sau khi gửi xong chuỗi, tác vụ gọi **xSemaphoreGive()** để giải phóng Mutex.
* Nhờ đó, dữ liệu gửi qua UART **không bị chồng lấn**, và **các chuỗi hiển thị tuần tự rõ ràng** trên terminal.

---

### **Kết quả mong đợi**

Trên cửa sổ UART (ở tốc độ 115200 bps), ta sẽ thấy các chuỗi lần lượt xuất hiện:

```
hello
xinchao
hello
xinchao
...
```

Chu kỳ lặp lại đều đặn **3 giây mỗi dòng**, chứng tỏ hai tác vụ hoạt động song song và được đồng bộ hóa đúng cách bằng Semaphore.

---




