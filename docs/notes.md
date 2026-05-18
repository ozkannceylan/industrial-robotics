# Çalışma Notları

> Öğrenme süresince biriken kavramlar, mekanikler, hata-dersleri.
> Kronolojik değil, konuya göre.

---

## 1. Colcon workspace yapısı

ROS2'de proje **paketler**'den oluşur, paketler bir **workspace** içinde yaşar. Workspace = `src/` dizinini içeren herhangi bir klasör. `colcon build` çalıştırıldığında üç sister dizin yaratılır:

| Dizin | Ne tutar |
|---|---|
| `src/` | **Senin yazdığın** orijinal kod (paket kaynakları). |
| `build/` | Ara build artifact'ları (object files, generated code, CMake cache). |
| `install/` | Build sonrası **kullanılabilir** halı: binary, header, share data, lifecycle metadata. ROS2 CLI burayı okur. |
| `log/` | colcon log kayıtları. |

**Kritik kural:** `ros2 run`, `ros2 interface show`, `ros2 launch` her zaman `install/` space'inden çalışır — **`src/` ile değil**. Source değiştirip rebuild etmezsen yeni hali "görünmez."

```bash
# Workspace root'tan (src/'nin bulunduğu dizin, içinden değil!):
colcon build --packages-select <pkg>     # yalnız belirtilen paketi build et
source install/setup.bash                # workspace'i shell'e tanıt
ros2 ...                                 # artık paket görünür
```

Her yeni shell oturumunda `source install/setup.bash` çağırmak gerekir (veya `.bashrc`'ye eklenir).

`.gitignore`'da `build/`, `install/`, `log/` ignore edilir — bunlar **tekrar üretilebilir**, repo'da yer kaplamamalı.

---

## 2. `irp_msgs` — interface paketi

### 2.1 ROS2'de üç iletişim primitive'i

| | Pattern | Yaşam süresi | Geri bildirim | İptal | Tipik kullanım |
|---|---|---|---|---|---|
| **Topic** (`.msg`) | publish/subscribe | sürekli stream | – | – | sensör, state, tf |
| **Service** (`.srv`) | request/response | anlık (ms) | yok | yok | quick query/command |
| **Action** (`.action`) | goal + feedback + result | uzun (s–dk) | periyodik | var | trajectory, navigation, manipülasyon |

Karar matrisi:
- "Sürekli akan veri" → topic
- "Tek soru, tek cevap, hemen biter" → service
- "Uzun süren iş, ilerlemesi izlensin, iptal edilebilsin" → action

### 2.2 `rosidl` pipeline — IDL ve code generation

**IDL = Interface Definition Language** (Arayüz Tanımlama Dili). Farklı dillerde (C++, Python, Java, ...) yazılmış yazılımların birbirleriyle nasıl haberleşeceğini belirleyen, **dilden bağımsız** bir şema standardı. Protobuf, Cap'n Proto, OpenAPI aynı kategoride.

**`rosidl` = ROS Interface Definition Language.** ROS2'nin kendi IDL tooling'i. Görevi: senin yazdığın `.msg` / `.srv` / `.action` dosyalarını alır ve birden çok dilin (C++, Python) anlayabileceği kaynak koduna dönüştürür.

Senin yazdığın `.msg` / `.srv` / `.action` dosyaları **kod değil, şema**. Build sırasında `rosidl` bunları okuyup şu çıktıları üretir:
- C++ header'ları (`#include "irp_msgs/msg/robot_mode.hpp"`)
- Python modülü (`from irp_msgs.msg import RobotMode`)
- Wire format type-support kütüphanesi (network'te serialize/deserialize)
- Introspection metadata'sı (`ros2 interface show`'un okuduğu şey)

3 satırlık `.msg`, build sonrası binlerce satır generated koda dönüşür.

### 2.3 `package.xml` — bağımlılık tipleri

| Tag | Anlamı | Analoji (Node.js) |
|---|---|---|
| `<buildtool_depend>X</buildtool_depend>` | Build sisteminin kendisi (`ament_cmake`, `cmake`) | build tool kurulumu |
| `<build_depend>X</build_depend>` | Bu paketi **derlerken** X gerekli. Çalıştırırken gerek yok. | `devDependencies` |
| `<exec_depend>X</exec_depend>` | Bu paketi **kullanırken** X gerekli (runtime). Derlerken gerek yok. | `dependencies` (sadece runtime) |
| `<depend>X</depend>` | İkisi birden. Kısayol. | `dependencies` (yaygın durum) |
| `<test_depend>X</test_depend>` | Sadece test'lerde gerekli. | `devDependencies` (test kısmı) |

`irp_msgs` için doğru ayrım:
- `rosidl_default_generators` → **`<build_depend>`** (generator kendisi, derleme bitince gereksiz)
- `rosidl_default_runtime` → **`<exec_depend>`** (generated kodun link ettiği kütüphane)

### 2.4 `member_of_group rosidl_interface_packages`

```xml
<member_of_group>rosidl_interface_packages</member_of_group>
```

**Üst seviye** olmalı (yani `<package>`'in doğrudan çocuğu), `<export>`'un içinde **DEĞİL**.

**Kural:** Standart, resmi tag'ler üst seviyede; tool'a özel ayarlar `<export>` içinde.

`<member_of_group>` ROS'un resmi metadata'sı (REP-149), `<export>` ise build/tool ekosistemlerinin "kendi tooling'ine mesaj bırakma" alanı. `rosidl_cmake` checker'ı `<export>` içine bakmıyor — yanlış yere koyarsan build:
```
Packages installing interfaces must include
'<member_of_group>rosidl_interface_packages</member_of_group>' in their
package.xml
```

### 2.5 `CMakeLists.txt` — `rosidl_generate_interfaces`

```cmake
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotMode.msg"
  "srv/SetRobotMode.srv"
  "action/MoveToHome.action"
  # DEPENDENCIES std_msgs  # eğer mesajlar başka paket tipini kullanıyorsa
)
```

- `find_package(...)` makroyu scope'a yükler. Olmazsa `rosidl_generate_interfaces` "unknown function" der.
- `rosidl_generate_interfaces` **`ament_package()`'tan ÖNCE** çağrılmalı. `ament_package()` paketi finalize ediyor; sonrasında yeni target eklenemez.
- `DEPENDENCIES` listesi: mesajın **içinde** başka paketin tipi kullanılıyorsa (örn. `std_msgs/Header header`) o paket buraya yazılır **ve** `package.xml`'e de `<depend>std_msgs</depend>` eklenir — iki yerde **birden** olmalı.

### 2.6 `.msg` syntax

```
# Constants — uppercase, type + name + = + value
uint8 MODE_IDLE    = 0
uint8 MODE_RUNNING = 1
uint8 MODE_FAULT   = 2
uint8 MODE_ESTOP   = 3

# Field — lowercase snake_case
uint8 mode
```

- **Constants** uppercase, `=` ile değer atanır. Compile-time integer constants, namespace altında erişilir (`irp_msgs::msg::RobotMode::MODE_RUNNING`).
- **Fields** snake_case, sadece tip + isim.
- **`#`** ile yorum.

Naming convention: **tip isimleri PascalCase, field isimleri snake_case**. `uint8 RobotMode` yazılırsa `RobotMode` field adı olur — okuyan kişi "bu bir tip mi?" diye karıştırır.

### 2.7 `.srv` syntax

```
# Request bölümü
RobotMode mode
---
# Response bölümü
bool success
string message
```

- Tek separator: **`---`** (üç tire, satır başında, yalnız).
- Üstte request (client → server), altta response (server → client).
- **Aynı paketten tip referansı:** sadece `RobotMode mode`. Tam yol (`irp_msgs/RobotMode`) **gerekmez**.
- **Farklı paketten tip referansı:** `std_msgs/Header header` gibi tam yol.

**Tipik pattern:** Response'ta `bool success + string message` — ROS2 ekosisteminde neredeyse evrensel konvansiyon. Opak error code yerine human-readable mesaj.

### 2.8 `.action` syntax

```
# Goal bölümü
float64 max_velocity_scaling
---
# Result bölümü
bool success
string message
float64 final_error
---
# Feedback bölümü
float64 percent_complete
```

- **İki** separator, **üç** bölüm.
- Sıra: **goal → result → feedback**.
- Herhangi bir bölüm tamamen boş olabilir (sadece comment veya hiçbir şey).

Bir `.action` dosyasından **üç mesaj tipi** üretilir: `<Ad>_Goal`, `<Ad>_Result`, `<Ad>_Feedback`. Ayrıca arka planda **altı kanal** kurulur:
- 3 service: send_goal, cancel_goal, get_result
- 2 topic: feedback, status

Tek dosya, altı endpoint — action protokolünün kaputu altı bu.

### 2.9 Type composition

`.srv` ve `.action` içinde mesaj tipleri referans edilebilir:

```
RobotMode mode          # aynı paketten
---
geometry_msgs/Pose pose # farklı paketten
```

Yararı:
- **Type safety** — client `uint8` değil, full `RobotMode` instance gönderir; sabitler tipe paketli gelir.
- **Wire format aynı** (`RobotMode` tek `uint8`'den ibaret → aynı byte'lar).
- **Self-documenting** — okuyan "bu bir mode" diye anlar.

`ros2 interface show` nested tipleri indent'leyerek expand eder:
```
RobotMode mode
        uint8 MODE_IDLE    = 0
        uint8 MODE_RUNNING = 1
        uint8 MODE_FAULT   = 2
        uint8 MODE_ESTOP   = 3
        uint8 mode
---
bool success
string message
```

---

## 3. Lifecycle node

### 3.1 Niye var?

Standart `rclcpp::Node` "constructor'da başla, destructor'da bit" modelinde — production için yetersiz:
- Hardware boot süresini bekleyemez (publish ederken sensör hazır değil → çöp veri)
- Deterministik startup sırası kurmak zor (perception → planner → controller)
- Runtime'da güvenli pause/resume yok (fault'ta controller dursun ama perception kalsın)
- Composable manager'lar koordine edemiyor

Çözüm: **managed lifecycle** — explicit state machine, transition'lar dış dünyadan tetiklenir.

### 3.2 State machine

```
unconfigured  ──configure──▶  inactive  ──activate──▶  active
                              ▲                       │
                              └──────deactivate───────┘
                              │
                          cleanup ──▶ unconfigured
                              │
                          shutdown ──▶ finalized
```

Beş primary transition: **configure, activate, deactivate, cleanup, shutdown**.

Her transition'ın callback'i:

| Callback | Tetikleyen | Ne yapılmalı |
|---|---|---|
| `on_configure` | configure | parametre oku, publisher/subscriber **oluştur** (henüz activate etme), hardware connect |
| `on_activate` | activate | publisher'ları `activate()` et, gerçek iş başlat |
| `on_deactivate` | deactivate | publisher'ları `deactivate()` et, resource'ları **memory'de bırak** |
| `on_cleanup` | cleanup | publisher/subscriber'ları **destroy** et, hardware disconnect, memory geri ver |
| `on_shutdown` | shutdown | final teardown |

**Kritik ayrım:** `inactive ↔ active` ucuz (sadece "pasif/aktif" anahtarlama), `unconfigured ↔ inactive` pahalı (alloc/dealloc). Sık tetiklenecek geçişleri ucuz tarafta tut.

**Constructor'ın rolü daraltılmış:** sadece dependency-free init (üye değişkenlerin default değerleri). Hardware'e dokunma, parametre okuma, publisher oluşturma → hepsi `on_configure`'a.

### 3.3 Lifecycle publisher

`rclcpp_lifecycle::LifecyclePublisher` — `inactive` iken `publish()` çağırsan bile mesaj **dropla**nır. State machine'i bypass ederek mesaj yayınlamayı engelliyor. Sadece `active` state'te gerçekten gönderir.

### 3.4 Process ≠ State

Önemli kavramsal nokta: lifecycle state ve OS process **bağımsız**.

`shutdown` transition'ı state'i `finalized`'a alır ama process'i exit etmez. Otomatik kapanmaz; senin `on_shutdown` veya `main()` "spin'i durdur ve `rclcpp::shutdown()` çağır" demek zorunda.

CLI'dan gözlem:
```bash
$ ros2 lifecycle set /lifecycle_demo shutdown
Transitioning successful
$ ros2 lifecycle get /lifecycle_demo
finalized [4]
$ ros2 lifecycle set /lifecycle_demo configure
Unknown transition requested, available ones are:    # liste boş — terminal state
```

`finalized` terminal state, çıkışı yok. Yeniden başlatmak için process'i kill edip yeniden spawn.

### 3.5 CLI keşfi

```bash
ros2 lifecycle nodes                          # tüm lifecycle node'lar
ros2 lifecycle get /<node>                    # mevcut state
ros2 lifecycle list /<node>                   # mevcut state'ten geçerli transition'lar
ros2 lifecycle set /<node> <transition>       # transition tetikle
```

`ros2 lifecycle list` state'e göre değişir — state machine'i CLI'dan canlı keşfetmek için iyi.

---

## 4. C++ patterns (ROS2 bağlamında)

### 4.1 Inheritance + constructor + member initializer list

```cpp
class LifecycleDemo : public rclcpp_lifecycle::LifecycleNode
{
public:
  LifecycleDemo()
  : rclcpp_lifecycle::LifecycleNode("lifecycle_demo")   // ← member init list
  {
    // constructor body
  }
};
```

- `: public Y` — public inheritance (ROS2'de standart).
- `public:` — access specifier (default `private`, ROS2 callback'leri public olmak zorunda).
- **Member initializer list** (`: Base("name")`) — constructor body'sinden ÖNCE çalışır, base class constructor'ına argüman geçer. Node adı **mutlaka burada** verilir (constructor body'sinde olmaz, base önce inşa edilmek zorunda).

### 4.2 `override` keyword

```cpp
CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
```

- `override` = "ben base class'taki virtual bir fonksiyonu override ediyorum."
- Yanlış yazarsan (parametre tipi tutmaz, isim tipo, vs.) derleme hatası → bug'ları erken yakalar.
- Optional ama **her zaman yaz**.

### 4.3 `const Type &` parametreler

```cpp
on_configure(const rclcpp_lifecycle::State &)
```

- `&` reference — kopyalama yok.
- `const` — fonksiyon bu objeyi değiştirmeyeceğine söz veriyor.
- **İsim yok** — parametre kullanılmıyorsa adlandırmak zorunda değilsin.

### 4.4 Smart pointer + `make_shared`

```cpp
auto node = std::make_shared<LifecycleDemo>();
```

- `std::make_shared<T>(...)` — heap'te `T` oluştur, `std::shared_ptr<T>` döndür.
- Reference-counted; son referans kaybolduğunda otomatik delete.
- ROS2'de manual `new`/`delete` neredeyse hiç kullanılmaz — her şey shared_ptr (veya unique_ptr).

### 4.5 `auto` keyword

Compile-time type deduction. Python'daki dynamic typing **değil**; compiler ifadenin tipini çıkarır, ama tip yine de sabit.

### 4.6 `->` vs `.`

- `obj.member` — obje veya reference'tan üye erişimi.
- `ptr->member` — pointer (raw veya shared_ptr) üzerinden üye erişimi. `(*ptr).member` ile aynı, kısayolu.

### 4.7 `using` type alias

```cpp
using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
```

C++11 syntax — eski `typedef`'in modern hali. Uzun namespace yollarını yerel kısa isimlere bağlamak için.

### 4.8 `rclcpp::spin` for lifecycle nodes

```cpp
rclcpp::spin(node->get_node_base_interface());  // ✓ lifecycle node
rclcpp::spin(node);                              // ✗ derleme hatası — lifecycle için değil
```

Lifecycle node, plain Node'dan farklı hiyerarşide. `spin()` overload'ları belirli interface tiplerini bekler; `get_node_base_interface()` doğru handle'ı verir.

---

## 5. CMake patterns (ROS2 / ament)

### 5.1 Executable target

```cmake
add_executable(<target_name> <source_files...>)
ament_target_dependencies(<target_name> <ros2_packages...>)
install(TARGETS <target_name> DESTINATION lib/${PROJECT_NAME})
```

- `add_executable` — binary tanımla.
- `ament_target_dependencies` — ROS paketlerini link et + include path'leri ekle. Düz CMake'de `target_include_directories` + `target_link_libraries` ile manuel yapılan işin kısayolu.
- `install(TARGETS ... DESTINATION lib/${PROJECT_NAME})` — `ros2 run <pkg> <bin>`'in arayacağı konvansiyonel yol.

### 5.2 `${PROJECT_NAME}` variable

CMake variable, `project(<name>)` ile set olur. Hardcode yerine değişken kullanmak — paket adını bir yerde değiştirince her şey güncellenir.

---

## 6. Yapılan hatalar ve dersler

| Hata | Tezahür | Düzeltme | Ders |
|---|---|---|---|
| `<member_of_group>`'u `<export>` içine koymak | CMake "must include `<member_of_group>...`" hatası | Üst seviyeye taşı | Resmi metadata top-level, tool config `<export>` içi |
| `colcon build`'i `src/` içinden çalıştırmak | `build/install/log` `src/` altında oluşur | Workspace root'tan çalıştır | colcon CWD'yi workspace sayar |
| Field adı PascalCase (`uint8 RobotMode`) | Build geçer ama isim çarpışması/karışıklık | snake_case kullan (`uint8 mode`) | Tip = PascalCase, field = snake_case |
| `ros2 pkg create --dependencies ... <pkgname>` ile pkgname'i dep olarak yazmak | `<depend>irp_examples</depend>` + `find_package(irp_examples REQUIRED)` self-dep | Manuel kaldır | Komut argümanlarını dikkatli oku |
| Source değişti, rebuild yapılmadı | `ros2 interface show` eski hali gösteriyor | `colcon build` yeniden | Source ↔ install ayrımı |
| `ros2 pkg create` interface-only pakete `include/`+`src/` açtı | Boş dizinler, niyet sinyali bozuk | Sil | Default boilerplate her zaman uygun değil |
| Yeni paket repo root'una oluştu, `src/` altında değil | colcon paketi bulamaz | `mv <pkg> src/` | `ros2 pkg create`'i hangi dizinden çağırdığın önemli |

---

## 7. Sık kullanılan komutlar

```bash
# Build
colcon build --packages-select <pkg>
colcon build                              # tüm paketler
colcon build --symlink-install            # source değişikliklerini install'a sembolik link et (Python için faydalı)

# Workspace tanıt
source install/setup.bash

# Interface keşfi
ros2 interface list
ros2 interface list | grep irp_msgs
ros2 interface show <pkg>/<type>/<Name>
ros2 interface proto <pkg>/<type>/<Name>  # JSON template

# Node + lifecycle
ros2 node list
ros2 run <pkg> <executable>
ros2 lifecycle nodes
ros2 lifecycle get /<node>
ros2 lifecycle list /<node>
ros2 lifecycle set /<node> <transition>

# Paket oluştur (src/ içinden çağır!)
ros2 pkg create --build-type ament_cmake --dependencies <deps...> <pkg_name>
```

---

## 8. Tamamlanan adımlar (P0)

- [x] `irp_msgs` paketi
  - `msg/RobotMode.msg` — `uint8 mode` + 4 sabit (IDLE/RUNNING/FAULT/ESTOP)
  - `srv/SetRobotMode.srv` — `RobotMode mode` request, `bool success + string message` response
  - `action/MoveToHome.action` — `float64 max_velocity_scaling` goal, `bool success + string message + float64 final_error` result, `float64 percent_complete` feedback
- [x] Lifecycle node örneği (`irp_examples/lifecycle_demo`)
  - 5 transition callback'i implement edildi (configure/activate/deactivate/cleanup/shutdown)
  - CLI'dan tüm transition'lar test edildi
  - `unconfigured → inactive → active → inactive → unconfigured → finalized` akışı doğrulandı

Sıradaki (P0 devamı):
- [ ] Layered launch sistemi (YAML config + Python composition)
- [ ] 3 farklı QoS profili demo
- [ ] UR5e xacro
- [ ] rviz'de joint hiyerarşisi
