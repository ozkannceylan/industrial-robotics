# TF2 + URDF: Repo Yapısı ve Açıklaması

> Bu doküman, `irp_description` paketindeki **URDF/xacro** ile `irp_examples`'taki
> **TF2** kodunu baştan sona açıklar: hangi dosya ne yapıyor, neden öyle yazıldı,
> nasıl çalıştırılır.

---

## 1. Eklenen dosyalar — kuşbakışı

```
src/
├── irp_description/                    ← robot tanımı paketi
│   ├── package.xml                     ament_cmake + xacro/rviz/rsp exec_depend'leri
│   ├── CMakeLists.txt                  derleme yok; sadece urdf/launch/rviz install
│   ├── urdf/
│   │   ├── common.xacro                yeniden kullanılabilir macro'lar
│   │   └── ur5e.urdf.xacro             UR5e kinematik zinciri (ana dosya)
│   ├── launch/
│   │   └── display.launch.py           xacro→rsp→jsp_gui→rviz zinciri
│   └── rviz/
│       └── ur5e.rviz                   hazır RViz görünümü (RobotModel + TF + Grid)
│
└── irp_examples/
    └── src/tf2_listener_demo_node.cpp  TF2 lookup (consumer) tarafı
```

**İki paketin iş bölümü:**
- `irp_description` = **broadcaster tarafı** (URDF → `robot_state_publisher` → `/tf`, `/tf_static`).
- `irp_examples/tf2_listener_demo` = **consumer tarafı** (`/tf`'i dinleyip `lookupTransform` ile sorgular).

Bu ayrım, URDF ve TF2'nin gerçek hayatta nasıl bağlandığını gösteriyor.

---

## 2. URDF / xacro tarafı

### 2.1 Neden ayrı `irp_description` paketi?

ROS2 konvansiyonu: robot **tanımı** (description) kendi paketinde yaşar, kod paketinden ayrı.
Sebep — **reuse + swap**: aynı description'ı simülasyon, MoveIt, gerçek donanım hepsi kullanır;
ROADMAP P8'de UR5e ↔ Franka swap'i "sadece `irp_description` + `irp_hardware` değişsin" diye
tasarlanmış. Description'ı izole tutmak bu hedefin altyapısı.

### 2.2 `common.xacro` — macro'lar (xacro'nun varlık sebebi)

İki macro var:

| Macro | Ne yapar | Neden |
|---|---|---|
| `cylinder_link` | İsim/yarıçap/uzunluk/kütle/materyal alıp tam bir `<link>` (visual+collision+inertial) üretir | 6 kol link'ini tek tek yazmak yerine tek şablon — **DRY** |
| `solid_cylinder_inertial` | Bir silindirin atalet tensörünü (rigid-body formülü) hesaplar | Gazebo dinamiği için; elle inertia yazmak hataya açık |

`cylinder_link`'in inceliği: silindir **+Z yönünde**, tabanı link orijininde duracak şekilde
yerleştirilmiş (`origin z = length/2`). Böylece bir sonraki joint "tam `z = length`" noktasına
oturur ve link'ler zincir boyunca **temiz istiflenir**. Bu, joint origin matematiğini basit tutar.

> **xacro kavramları:** `property` = değişken, `macro` = parametreli şablon, `include` = dosya
> parçalama, `${...}` = ifade (matematik dahil). Build/launch'ta `xacro` aracı bunları **düz URDF'e
> expand eder**; ROS aslında hep düz URDF görür.

### 2.3 `ur5e.urdf.xacro` — kinematik zincir

**Tasarım kararları (dosyanın başındaki yorumda da yazıyor):**
- **Segment uzunlukları** UR5e datasheet değerleri: `d1=0.1625, a2=0.425, a3=0.3922, d5=0.0997, d6=0.0996` m.
- **Joint eksenleri** gerçek UR5e desenini izler: **Z, Y, Y, Y, Z, Y**.
- **Visual'lar primitive** (cylinder/box) — vendor mesh'i yok. Amaç kinematik olarak dürüst,
  tanınabilir bir kol; foto-gerçekçilik değil.
- Link'ler +Z boyunca istiflenir → `joint_state_publisher_gui` slider'ları sezgisel hareket ettirir.

**Üretilen ağaç** (`check_urdf` ile doğrulandı):
```
base_link
└─ shoulder_link        (shoulder_pan_joint,  revolute, axis Z)
   └─ upper_arm_link    (shoulder_lift_joint, revolute, axis Y)
      └─ forearm_link   (elbow_joint,         revolute, axis Y)
         └─ wrist_1_link(wrist_1_joint,       revolute, axis Y)
            └─ wrist_2_link (wrist_2_joint,   revolute, axis Z)
               └─ wrist_3_link (wrist_3_joint,revolute, axis Y)
                  └─ tool0          (tool0_fixed_joint,  FIXED)
                     └─ camera_link (camera_mount_joint, FIXED)
```
9 link, 8 joint (6 revolute + 2 fixed).

**`tool0`** — zero-size link (sadece `<link name="tool0"/>`). UR konvansiyonunda tool flange frame'i.
Geometrisi yok çünkü o sadece bir **frame** (koordinat sistemi); buraya gripper/sensör bağlanır.

**`camera_link` + `camera_mount_joint` (type=fixed)** — **static transform** örneği. Kamera, `tool0`'a
**sabit** offset'le (`xyz="0.05 0 0.02" rpy="0 -π/2 0"`) bağlı. Fixed joint olduğu için
`robot_state_publisher` bunu `/tf` değil **`/tf_static`**'e yayınlar. Bir sensör montajının
("sensor mount = static TF") birebir karşılığı.

> **URDF → TF köprüsü (en önemli kavram):** Her `<joint>`, TF ağacında bir kenardır
> (`parent_link → child_link`). `robot_state_publisher` URDF'i + `/joint_states`'i okur,
> **revolute** joint'ler için açıya göre transform hesaplayıp `/tf`'e, **fixed** joint'ler için
> sabit transform'u `/tf_static`'e yayınlar. Yani URDF yazmak = TF ağacının statik iskeletini tanımlamak.

### 2.4 `display.launch.py` — görselleştirme zinciri

Dört adım:
1. **`xacro` işle:** `Command(["xacro ", model])` ile launch anında xacro→URDF string. `ParameterValue(..., value_type=str)` şart — sonuç düz string parametre olarak geçsin diye.
2. **`robot_state_publisher`:** URDF'i `/robot_description`'a, transform'ları `/tf` + `/tf_static`'e yayınlar.
3. **`joint_state_publisher_gui`:** slider penceresi; joint açılarını `/joint_states`'e yayınlar (`gui:=false` ile kapatılabilir).
4. **`rviz2`:** hazır `ur5e.rviz` config ile açılır (RobotModel + TF + Grid, fixed frame `base_link`).

---

## 3. TF2 tarafı — `tf2_listener_demo_node.cpp`

URDF/`robot_state_publisher` transform'ları **yayınlar**; bu node onları **okur**.

**İskelet:**
```cpp
tf_buffer_   = std::make_shared<tf2_ros::Buffer>(get_clock());     // cache + interpolation
tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);  // /tf + /tf_static dinler
// ...
t = tf_buffer_->lookupTransform(target_frame_, source_frame_, tf2::TimePointZero);
```

**Kritik noktalar (kod yorumlarında da var):**
- **Sıra önemli:** önce `Buffer`, sonra `TransformListener` (listener buffer'a yazar).
- **`lookupTransform(TARGET, SOURCE, TIME)`** — "SOURCE frame'indeki noktayı TARGET'ta ifade eden
  transform". Default sorgu: `camera_link`'in `base_link`'teki yeri = wrist kamerasının robot
  tabanına göre konumu.
- **`tf2::TimePointZero`** = "en son mevcut transform" — zaman senkronu derdi yok, en dayanıklı.
- **`try/catch (tf2::TransformException&)`** — her lookup sarılır. Frame henüz yok / ağaç bağlı değil /
  o zaman için veri yok durumlarında exception. Gerçek sistemde burada **fail-safe** yaparsın
  (eski/uydurma pose ile asla hareket etme).
- **Parametrik:** `target_frame` / `source_frame` ROS parametreleri, runtime'da değiştirilebilir.

---

## 4. Build & çalıştırma

```bash
cd ~/projects/industrial-robotics
colcon build --packages-select irp_description irp_examples
source install/setup.bash

# 1) Robotu RViz'de göster (slider'larla joint'leri oynat)
ros2 launch irp_description display.launch.py

# 2) Başka bir terminalde: TF lookup demo (transform'lar yayınlanırken)
source install/setup.bash
ros2 run irp_examples tf2_listener_demo
#   -> "camera_link in base_link | pos=[...] quat=[...]" her saniye

# Farklı bir transform sorgula:
ros2 run irp_examples tf2_listener_demo --ros-args -p source_frame:=tool0

# 3) TF ağacını incele:
ros2 run tf2_tools view_frames        # frames.pdf üretir
ros2 topic echo /tf_static            # camera + tool0 sabit transform'larını gör
ros2 run tf2_ros tf2_echo base_link camera_link
```

**Doğrulama durumu:**
- `colcon build` — iki paket de uyarısız derlendi. ✅
- `xacro ... | check_urdf` — XML geçerli, ağaç beklenen seri zincir. ✅
- RViz görsel doğrulama — RobotModel + TF Status: Ok, 6 joint slider ile doğrulandı. ✅

---

## 5. RViz görsel doğrulama checklist

`ros2 launch irp_description display.launch.py` sonrası:
- [ ] RobotModel görünüyor, kırmızı hata yok.
- [ ] Fixed Frame `base_link`; her link'te TF eksenleri görünüyor.
- [ ] `joint_state_publisher_gui` slider'ları → ilgili link doğru eksende dönüyor
      (shoulder_pan yatayda süpürür, shoulder_lift kolu kaldırır, vb.).
- [ ] `camera_link` (kırmızı kutu) wrist ucunda, tool0'a sabit; kol hareket edince onunla gider.
- [ ] TF ağacında zincir kopuk değil (base_link → ... → camera_link tek dal).

> **Not (QoS):** `robot_state_publisher`, `/robot_description`'ı **transient_local** (latched) yayınlar.
> RViz RobotModel display'inin "Description Topic" durability'si de **Transient Local** olmalı, yoksa
> RViz publish'ten sonra açıldığında (late-join) latched mesajı alamaz ve model görünmez.

---

## 6. Sonraki adımlar

- Bu model **primitive geometry** kullanıyor; ileride (opsiyonel) gerçek UR5e mesh'leri eklenebilir.
- P1'de `ros2_control` tag'leri bu xacro'ya eklenecek (şimdilik yok — bilinçli).
- Joint origin'leri öğrenme-modeli; tam DH-uyumlu UR5e pose'u P2 (kinematik) işinde netleşecek.
- **Teknik borç:** `base_link`'e inertia konuldu; KDL root link'te inertia istemiyor
  ("KDL does not support a root link with an inertia" uyarısı). P1/P2 öncesi `base_link`'i
  inertia'sız yapıp altına `base_link_inertia` eklemek temizler.
