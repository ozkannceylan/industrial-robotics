# Modern C++ Patterns: Repo Yapısı ve Açıklaması

> `irp_examples` paketine eklenen iki entegrasyon node'unun haritası: hangi dosya
> hangi C++ kavramını gösteriyor, nasıl çalıştırılır, ne doğrulandı.

---

## 1. Eklenen dosyalar

```
src/irp_examples/
├── src/command_safety_gate_node.cpp        topic+service+param+RAII entegrasyonu
├── src/static_mount_broadcaster_node.cpp   programatik static TF
├── CMakeLists.txt                          + 2 target, + std_srvs / tf2 find_package
└── package.xml                             + <depend>std_srvs</depend>, <depend>tf2</depend>
```

İkisi de mevcut `irp_examples` paketine eklendi (yeni paket gerekmedi).

---

## 2. `command_safety_gate_node.cpp` — entegrasyon node'u

**Ne yapıyor:** Minimal bir "safety gate". `/cmd_vel_in` (Twist) dinler, `linear.x`'i `±max_speed`'e
**clamp** edip `/cmd_vel_out`'a republish eder. `/reset` (Trigger) servisi işlenen komut sayacını
sıfırlar. `max_speed` parametresi runtime'da değişebilir; negatifse **reddedilir**.

**Hangi C++ kavramı nerede:**

| Kavram | Kod | Satır mantığı |
|---|---|---|
| **shared_ptr** | tüm `pub_/sub_/reset_srv_` üyeleri `...::SharedPtr`; `make_shared` in `main` | her rclcpp handle reference-counted |
| **RAII / lock_guard** | `on_cmd`, `on_reset`, `on_param_update` içinde `std::lock_guard<std::mutex> lock(mtx_)` | `mtx_` ile `processed_` + `max_speed_` korunur; scope çıkışında otomatik unlock |
| **const correctness** | `on_cmd(const Twist & in)`, `const double v` | kopya yok + değişmez |
| **std::bind vs lambda** | sub = **lambda**, service + param cb = **std::bind** | kısa inline → lambda; named member (özellikle 2-arg service) → bind |
| **template** | `create_publisher<Twist>`, `create_service<Trigger>` | compile-time tip |
| **param validation** | `add_on_set_parameters_callback` → `result.successful=false` reddeder | negatif `max_speed` reddi |
| **std::clamp** | `std::clamp(in.linear.x, -limit, limit)` | C++17 standart algoritma |

**Neden mutex?** `MultiThreadedExecutor` altında topic callback'i ve service callback'i **farklı
thread'lerde** koşabilir; ikisi de `processed_`'a dokunur → **data race**. `lock_guard` ile korunur.
SingleThreadedExecutor altında şart değil ama doğru alışkanlık + thread-safe-by-design.

**Çalıştır & dene:**
```bash
ros2 run irp_examples command_safety_gate
# başka terminal:
ros2 topic pub /cmd_vel_in geometry_msgs/msg/Twist "{linear: {x: 5.0}}"
ros2 topic echo /cmd_vel_out                       # linear.x -> max_speed'e clamp'li
ros2 param set /command_safety_gate max_speed 2.0  # kabul
ros2 param set /command_safety_gate max_speed -1.0 # REDDEDİLİR
ros2 service call /reset std_srvs/srv/Trigger      # sayaç sıfırla
```

---

## 3. `static_mount_broadcaster_node.cpp` — programatik static TF

**Ne yapıyor:** `base_link -> lidar_link` sabit transform'unu **kod ile** yayınlar (URDF/
`robot_state_publisher` yerine). Translation (1.5, 0, 1.8) + rotation (RPY→quaternion). Bu, TF'in
broadcaster tarafı; `tf2_listener_demo` ise consumer tarafı — ikisi TF'i tamamlar.

**Önemli noktalar:**
- `tf2_ros::StaticTransformBroadcaster` — arka planda **latched** (`transient_local` QoS) bir
  `/tf_static` publisher tutar → geç katılan listener da alır.
- `tf2::Quaternion q; q.setRPY(...)` — TF oryantasyonu **her zaman quaternion** tutar (gimbal lock
  yok, temiz interpolation). RPY'den quaternion'a çevirip `t.transform.rotation`'a kopyalanır.
- `t.header.stamp = get_clock()->now()`, `frame_id` (parent) + `child_frame_id` (child) doldurulur.
- `spin` edilir ki latched transform listener'lar için yayında kalsın.

**Çalıştır & dene:**
```bash
ros2 run irp_examples static_mount_broadcaster
# başka terminal:
ros2 run irp_examples tf2_listener_demo --ros-args -p target_frame:=base_link -p source_frame:=lidar_link
ros2 run tf2_ros tf2_echo base_link lidar_link     # Translation: [1.5, 0, 1.8]
ros2 topic echo /tf_static
```

---

## 4. Doğrulama

- ✅ `colcon build --packages-select irp_examples` — uyarısız derlendi.
- ✅ `command_safety_gate` runtime: `max_speed=-1.0` reddedildi ("must be >= 0"), `2.0` kabul,
  parameter callback log'u doğru.
- ✅ `static_mount_broadcaster` runtime: `tf2_echo base_link lidar_link` → `Translation: [1.500,
  0.000, 1.800]`, identity rotation — kodlandığı gibi.

---

## 5. Repo'daki tüm `irp_examples` node'ları

| Node | Gösterdiği |
|---|---|
| `lifecycle_demo` | managed lifecycle, 5 transition, lifecycle publisher |
| `qos_demo` | 3 QoS profili + mismatch + late-join |
| `tf2_listener_demo` | Buffer+Listener, `lookupTransform`, try/catch |
| `command_safety_gate` | smart_ptr + RAII mutex + params + topic + service |
| `static_mount_broadcaster` | programatik static TF, RPY→quaternion |
| (`irp_description`) | UR5e xacro → robot_state_publisher → /tf |

Hepsi `ros2 run irp_examples <node>` ile çalışır (önce `colcon build` + `source install/setup.bash`).

---

## 6. Bilinen teknik borç

- `command_safety_gate` thread-safety'si `MultiThreadedExecutor` varsayımıyla yazıldı; default
  `spin` single-threaded. Gerçek RT senaryoda callback group + executor tuning gerekir.
- `base_link` inertia uyarısı hâlâ açık — KDL root link'te inertia istemiyor (bkz. `tf2_urdf_structure.md`).
