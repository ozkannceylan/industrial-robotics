# `qos_demo_node.cpp` — Satır Satır Anlatım

> [src/irp_examples/src/qos_demo_node.cpp](../../src/irp_examples/src/qos_demo_node.cpp) dosyasını **satır satır** açıklar.
> Yan ürün: C++ syntax crash course (yalnız bu dosyada geçen kavramlar).
>
> Hedef: dosyayı kapattığında "her satırın **niye** orada olduğunu" söyleyebilmek.

---

## 0. Bu node ne yapıyor — 30 saniye özeti

Tek node, üç publisher + üç subscriber (+ bir mismatch sub). Her publisher farklı bir **QoS profili** kullanıyor:

| Topic    | Reliability | History     | Durability        | Senaryo                        |
| -------- | ----------- | ----------- | ----------------- | ------------------------------ |
| sensor   | BestEffort  | KeepLast(1) | Volatile          | Yüksek frekans, kayıp tolere   |
| command  | Reliable    | KeepLast(10)| Volatile          | Komut, kaybedilemez            |
| config   | Reliable    | KeepLast(1) | **TransientLocal**| Geç bağlanan abone da almalı   |

Bonus: `sensor` topic'ine **kasten uyumsuz** (Reliable) bir subscriber bağlanır → DDS uyarısı + mesaj akmaz.

Stats timer her 2 saniyede tx/rx sayaçlarını yazar.

---

## 1. C++ Crash Course (sadece bu dosyada geçenler)

Senior programmer olduğunu varsayıyorum; **Node.js/Python** referansıyla anlatıyorum. Tek başına yeterli bir C++ kursu **değil** — burada hangi tuhaflığa rastladığını anlamak için yeterli.

### 1.1 `#include`

```cpp
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
```

JS'deki `require`/`import`'un atası, ama **tamamen metinsel**. Preprocessor dosyayı bulup içeriğini bu satırın yerine yapıştırır. `<...>` standart/sistem yolları, `"..."` proje-içi yollar için (gevşek bir konvansiyon).

- `.hpp` = header file. Sınıf bildirimleri, template tanımları, inline fonksiyonlar burada.
- ROS2'de `rclcpp/rclcpp.hpp` "rclcpp'nin her şeyini" çeken **convenience header**'dır.

### 1.2 `namespace` + `using`

```cpp
using namespace std::chrono_literals;
using StringMsg = std_msgs::msg::String;
```

- **namespace** = isim kovası (Python'daki module/Java'daki package gibi). `std::chrono::milliseconds` → `std` ana namespace, `chrono` alt namespace, `milliseconds` tip.
- `using namespace X;` → o namespace'in içindekileri sanki o scope'a aitmiş gibi kullanabilirsin. Tehlikeli — global'de yapma, ad çakışması yaratır.
- `using A = B;` → **type alias** (TypeScript `type` gibi). `StringMsg` yazınca `std_msgs::msg::String` demek olur. Sadece okunabilirlik için.
- `chrono_literals` özel bir namespace: içeride `10ms`, `2s` gibi **user-defined literal**'lar var. `using` etmezsen `10ms` derlemez.

### 1.3 `class` ve erişim belirleyiciler

```cpp
class QosDemo : public rclcpp::Node
{
public:
  QosDemo() : rclcpp::Node("qos_demo") { ... }
private:
  rclcpp::Publisher<StringMsg>::SharedPtr sensor_pub_;
};
```

- `class X : public Y` → `X` `Y`'den **kalıtım** alır (`extends Y` gibi). `public` burada "Y'nin public üyeleri X'te de public" anlamında.
- `public:` / `private:` → onların **altına** yazılan üyelerin erişim düzeyini belirler. Bir kez `public:` yazılınca yenisi yazılana kadar tüm üyeler public.
- Default class içinde `private:`'dır (struct'ta `public:`).
- C++'da convention: private üyeler trailing underscore (`sensor_pub_`). `_isim` ve `__isim` C++ standardı için **reserved** — kullanma.

### 1.4 Constructor + member initializer list

```cpp
QosDemo()
: rclcpp::Node("qos_demo")
{
  RCLCPP_INFO(get_logger(), "...");
}
```

- `QosDemo() : rclcpp::Node("qos_demo")` — `:`'dan sonraki kısma **member initializer list** denir. Süslü parantez `{}` açılmadan önce çalışır.
- Burada özellikle önemli: parent class `rclcpp::Node`'un **constructor**'unu çağırıyoruz. Node'un default constructor'ı yok; ismi olmak zorunda.
- JS analojisi: `class QosDemo extends Node { constructor() { super("qos_demo"); ... } }`. `super()` çağrısı initializer list'tir.
- Member alanları da burada **doğrudan inşa edilir** (atama değil inşa). Performans + `const`/reference üye desteği için kritik.

### 1.5 Template'ler

```cpp
rclcpp::Publisher<StringMsg>::SharedPtr
create_publisher<StringMsg>("sensor", sensor_qos);
```

- `<>` içine **tip parametresi** verirsin. Compile-time generic. TypeScript generics ile aynı zihinsel model, ama tamamen derleme zamanı; runtime'da `Publisher<StringMsg>` ile `Publisher<Int32>` **farklı tiplerdir**.
- `create_publisher<StringMsg>("sensor", qos)` — "StringMsg yayınlayan bir publisher yarat" demek.
- `::SharedPtr` — sınıf içinde tanımlı nested type alias (genelde `using SharedPtr = std::shared_ptr<X>;` şeklinde).

### 1.6 Smart pointers — `shared_ptr`

```cpp
rclcpp::Publisher<StringMsg>::SharedPtr sensor_pub_;
std::make_shared<QosDemo>();
```

- C++'da object'in yaşam süresini yönetmek **manuel**. `new` ile heap'te yaratırsan `delete` ile silmen lazım, unutursan memory leak.
- `std::shared_ptr<T>` — reference-counted akıllı pointer. Kaç tane kopyası varsa onu sayar, son kopya yok olunca object'i `delete` eder. Python/JS'in GC'sine yakın bir his ama deterministik.
- `std::make_shared<T>(args...)` — `T`'yi heap'te yaratıp `shared_ptr<T>` döndürür. `new T(...)`'yu doğrudan kullanmak yerine bunu tercih et (allocation optimization + exception safety).
- Bu dosyada **tüm** ROS2 publisher/subscriber/timer `shared_ptr` olarak tutulur — çünkü ROS2 executor da onlara referans tutar; senin değişkenin sahip değil, **ortak sahibi**.

### 1.7 `auto` keyword

```cpp
auto sensor_qos = rclcpp::QoS(rclcpp::KeepLast(1)).best_effort();
```

- `auto` — derleyici sağ taraftaki ifadenin tipini çıkartıp değişkene atar (TypeScript `let x = ...`, Go `x := ...`).
- Uzun tip isimlerini saklamak için yararlı. Burada `sensor_qos`'un tipi `rclcpp::QoS` — yazsan da olurdu, ama `auto` daha temiz.

### 1.8 `chrono` literal'leri

```cpp
using namespace std::chrono_literals;
create_wall_timer(10ms, [this]() { ... });
create_wall_timer(3s,  [this]() { ... });
```

- `10ms`, `200ms`, `3s` → `std::chrono::milliseconds(10)` vb. tiplerine dönen kullanıcı-tanımlı literal'ler.
- `using namespace std::chrono_literals;` yapmadan derlemez.
- `chrono` library = C++'ın tip-güvenli süre/zaman kütüphanesi. Bir `seconds`'ı `milliseconds`'a verirsen derleyici otomatik dönüştürür; ama `int`'ten `seconds`'a doğrudan dönüşüm yoktur (kasıtlı bir kontrol).

### 1.9 Lambda'lar ve capture list — **bu dosyanın en kritik kısmı**

```cpp
sensor_sub_ = create_subscription<StringMsg>("sensor", sensor_qos,
    [this](const StringMsg::SharedPtr) { sensor_rx_count_++; });
```

- `[capture](params) -> ReturnType { body }` → bir **closure** tanımlar. JS arrow function'a benzer ama capture açıkça belirtilir.
- `[capture]` listesi: lambda'nın hangi dış değişkenlere, **nasıl** erişebileceğini söyler:
  - `[]` — hiçbir şey capture etme.
  - `[x]` — `x`'i value (kopya) ile capture et.
  - `[&x]` — `x`'i reference ile capture et (dışarıdaki orijinaline link).
  - `[this]` — class member'larına erişebilmek için `this` pointer'ını capture et.
  - `[=]` — tüm değişkenleri value ile capture et (gri zihin, açık liste tercih et).
  - `[&]` — tüm değişkenleri reference ile capture et (tehlikeli).

**JS analoji + farkı:** JS'de closure otomatik. C++'da **explicit** — ne capture ederim, kopya mı reference mı, hepsini yazarsın. Çünkü stack'te yaratılan değişken stack frame ölünce yok olur — eğer reference capture ettiysen ve frame öldükten sonra lambda çalışırsa **dangling reference** (UB). Bu yüzden lambda'nın **ne zaman** çalışacağı kritik.

Bu dosyadaki örnekler:

| Yer | Capture | Niye |
|---|---|---|
| sensor_sub_ callback | `[this]` | `sensor_rx_count_++` member, `this` lazım |
| sensor_timer_ callback | `[this]` | `sensor_pub_`, `sensor_tx_count_` member; `sensor_qos`'a ihtiyaç yok |
| late_sub_setup_timer_ | `[this, config_qos]` | `config_qos` local değişken, timer fire ettiğinde constructor scope kapanmış olacak — value capture şart |

> `[this, &config_qos]` deseydin: timer 3 saniye sonra fire ettiğinde `config_qos` çoktan stack'ten silinmiş olurdu → **undefined behavior**. Crash veya rastgele bytes okumak gibi.

### 1.10 Çeşitli tuhaflıklar

- `const StringMsg::SharedPtr` parametre → callback'e gelen mesaj. `SharedPtr` zaten kopyalanabilir; `const` "ben bu pointer'ı değiştirmem" diyor (içeriği değil — ona `SharedPtr<const StringMsg>` gerekir).
- `m.data = "sensor#" + std::to_string(...)` — `std::string` operator overload'lı, `+` concatenation. `to_string` int → `std::string`.
- `std::make_shared<QosDemo>()` ile node oluştur, `rclcpp::spin` ile event loop'a sok. `spin` blokludur; SIGINT gelene kadar callback'leri çalıştırır.

---

## 2. ROS2 QoS — minimal teori

Üç ekseni var (DDS'in 20+ policy'sinden ROS2'nin sıkça kullandığı 3'ü):

### 2.1 Reliability
- **BestEffort:** "Gönder, kaybolursa kaybolsun, retry yok." Düşük gecikme, kayıp tolere.
- **Reliable:** "ACK gelene kadar retry et, ulaştır." Gecikme + bandwidth maliyeti.

### 2.2 History (Depth + Kind)
- **KeepLast(N):** Bellekte son N mesajı tut. Slow consumer yetişemezse en eski düşer.
- **KeepAll:** Hiç düşürme (RMW limit'ine kadar). Pratikte nadiren.

### 2.3 Durability
- **Volatile:** Pub geçmişini saklamaz. Subscribe etmeden önce yayınlanan mesajlar **kayıp**.
- **TransientLocal:** Pub son N (= history depth) mesajı saklar; **geç bağlanan** subscriber'a yollar.

### 2.4 Compatibility kuralı

`pub ≥ sub` (her eksende). Yani publisher offer'ı subscriber'ın request'ini **karşılayabilmeli**.

- Reliable pub + BestEffort sub → eşleşir
- BestEffort pub + Reliable sub → **eşleşmez**
- TransientLocal pub + Volatile sub → eşleşir
- Volatile pub + TransientLocal sub → **eşleşmez**

DDS discovery uyumsuzluğu fark edince `requested QoS profile is incompatible` uyarısı basar ve **mesaj akmaz**.

### 2.5 Builder pattern

```cpp
auto qos = rclcpp::QoS(rclcpp::KeepLast(1))
            .best_effort()
            .durability_volatile();
```

- `rclcpp::QoS` constructor'ı History alır (depth + kind).
- Diğer ayarlar **chained setter** çağrılarıyla geliyor: her setter `*this`'i döndürür → zincirleme.
- Pattern adı: **fluent interface / builder pattern**. Java `StringBuilder`, JS `array.map(...).filter(...)` gibi.

---

## 3. Dosya, satır satır

### 3.1 Header yorum bloğu (satır 1–10)

```cpp
// QoS Demo Node
// ----------------------------------------------------------------------------
// 3 topic, 3 farkli QoS profili:
//   sensor   -> BestEffort + KeepLast(1)  + Volatile         (hizli, kayipli)
//   command  -> Reliable   + KeepLast(10) + Volatile         (kaybedilemez)
//   config   -> Reliable   + KeepLast(1)  + TransientLocal   (gec-katilan da alir)
//
// Bonus: sensor topic'ine kasten "Reliable" subscriber bagla -> QoS mismatch
// (rclcpp uyari basar, mesaj akmaz).
// ----------------------------------------------------------------------------
```

Dosya başı dokümantasyon. C++'da Doxygen formatı (`///`, `/** */`) de yaygın ama burası eğitim node'u, basit `//` yorumlar yeterli.

### 3.2 Include'lar (12–13)

```cpp
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
```

- `rclcpp/rclcpp.hpp` — `rclcpp::Node`, `rclcpp::QoS`, `rclcpp::Publisher`, `rclcpp::spin`, `rclcpp::init`, vs. Hepsini tek seferde çeken convenience header.
- `std_msgs/msg/string.hpp` — generated header. ROS2'nin standart `std_msgs/String` mesaj tipi. Build sırasında `rosidl` üretir. Build edilmemiş bir mesaj tipini include edersen `file not found` alırsın.

### 3.3 Namespace ve type alias (15–16)

```cpp
using namespace std::chrono_literals;
using StringMsg = std_msgs::msg::String;
```

- `chrono_literals` → `10ms`, `200ms`, `2s`, `3s` literal'leri.
- `StringMsg` → her yerde `std_msgs::msg::String` yazmamak için kısaltma. Tamamen estetik.

> **`using namespace` global'de neden tehlikeli?** Bütün dosya boyunca `std::chrono`'nun içindeki tüm isimler "kapıyı çalmadan" girer. Adın çakışırsa derleme ya da semantic hata. Burada problem yok çünkü `chrono_literals` çok dar bir namespace (yalnızca `_ms`, `_s`, `_ns` vb. operator'lar). Genel kuralda **fonksiyon scope** içinde kullan veya hiç kullanma.

### 3.4 Class bildirimi (18–20)

```cpp
class QosDemo : public rclcpp::Node
{
public:
```

- `rclcpp::Node`'dan kalıtım → ROS2 node altyapısını (parameter, logging, executor handoff) bedavaya alırız.
- `public:` → bundan sonra constructor.

### 3.5 Constructor başı (21–23)

```cpp
QosDemo()
: rclcpp::Node("qos_demo")
{
  RCLCPP_INFO(get_logger(), "QosDemo kuruluyor");
```

- Constructor parametresiz. Initializer list'te parent constructor'a node adı veriliyor — bu ad `ros2 node list`'te görünecek.
- `RCLCPP_INFO(logger, fmt, ...)` → ROS2'nin printf-style logger macro'su. `get_logger()` parent Node'dan gelen helper.

### 3.6 Sensor QoS + publisher + subscriber + timer (28–50)

```cpp
auto sensor_qos = rclcpp::QoS(rclcpp::KeepLast(1))
                    .best_effort()
                    .durability_volatile();

sensor_pub_ = create_publisher<StringMsg>("sensor", sensor_qos);

sensor_sub_ = create_subscription<StringMsg>("sensor", sensor_qos,
        [this](const StringMsg::SharedPtr) { sensor_rx_count_++; });

sensor_timer_ = create_wall_timer(10ms, [this]() {
  StringMsg m;
  m.data = "sensor#" + std::to_string(sensor_tx_count_++);
  sensor_pub_->publish(m);
});
```

Satır satır:

1. `rclcpp::QoS(rclcpp::KeepLast(1))` — QoS object'i, History=KeepLast(1) ile inşa.
2. `.best_effort()` — Reliability=BestEffort. Setter `*this` döndürür → zincirleme.
3. `.durability_volatile()` — Durability=Volatile. (Aslında default zaten Volatile, açıkça yazmak doküman.)
4. `create_publisher<StringMsg>("sensor", sensor_qos)` — base class Node'dan gelen metod. "Bu topic'e bu tipte mesaj yayınla, bu QoS ile." Geriye `shared_ptr<Publisher<StringMsg>>` döndürür → member `sensor_pub_`'a atanır.
5. `create_subscription<StringMsg>("sensor", sensor_qos, callback)` — aynı topic'e abone. **Callback** mesaj her geldiğinde çağrılır.
6. Lambda parametresi `const StringMsg::SharedPtr` — gelen mesaj. İsim vermedik (`msg` yazmadık) çünkü kullanmıyoruz; sadece sayaç artırıyoruz. C++17+ ile parametre ismi opsiyonel.
7. `create_wall_timer(10ms, lambda)` — duvar saatine göre (wall clock, simulation time'a *değil*) 10ms'de bir lambda'yı çalıştırır.
8. Lambda gövdesi: yeni `StringMsg` yarat, `data` alanına string yaz, publisher ile yayınla. `sensor_tx_count_++` **post-increment** — önce mevcut değeri kullan, sonra artır. İlk mesaj `"sensor#0"`.

> Neden self-publish + self-subscribe? Aynı node hem yayıncı hem abone — tx/rx farkını mismatch'sız bir baseline olarak görelim diye. Aynı QoS'la her zaman eşleşir.

### 3.7 Command (55–68)

`sensor`'la birebir aynı pattern, sadece QoS profili farklı:

```cpp
auto command_qos = rclcpp::QoS(rclcpp::KeepLast(10))
                    .reliable()
                    .durability_volatile();
```

- `KeepLast(10)` — slow consumer durumunda 10 mesaj buffer; Reliable retry'ı bekleyen mesajları buffer'da tutmak için tipik.
- 200ms periyot → 5Hz, "komut" ritmi.

### 3.8 Config + late-join testi (79–98)

```cpp
auto config_qos = rclcpp::QoS(rclcpp::KeepLast(1))
                        .reliable()
                        .transient_local();
config_pub_ = create_publisher<StringMsg>("config", config_qos);

StringMsg config_msg;
config_msg.data = "config#1";
config_pub_->publish(config_msg);

late_sub_setup_timer_ = create_wall_timer(3s, [this, config_qos]() {
    config_sub_ = create_subscription<StringMsg>("config", config_qos,
        [this](const StringMsg::SharedPtr msg) {
            RCLCPP_INFO(get_logger(), "Late subscriber received: '%s'", msg->data.c_str());
        });
    late_sub_setup_timer_->cancel();
});
```

İşin **kalbi**. Akış:

1. `config_qos` = Reliable + KeepLast(1) + **TransientLocal**.
2. Publisher hemen yaratılır.
3. **Hemen `config#1` yayınlanır** — henüz hiç subscriber yok.
4. 3 saniyelik bir timer kurulur. Lambda **value capture** ile `this` + `config_qos`.
5. Timer fire edince **subscriber yaratılır**. Pub TransientLocal olduğu için, son yayınlanan mesaj (`config#1`) bellektedir → DDS sub eşleşir eşleşmez kopyayı sub'a iletir.
6. Iç callback `RCLCPP_INFO` ile "Late subscriber received: 'config#1'" basar.
7. `late_sub_setup_timer_->cancel()` → timer durur, bir daha fire etmez.

> **`[this, config_qos]` neden value capture?** `config_qos` constructor scope'unda yaratılan local değişken. Constructor `{...}` bloğu kapandığında (yani node tam inşa olduktan sonra) stack'teki bu değişken ölür. Lambda 3 saniye sonra fire edecek — o anda `config_qos` zaten yok. Reference capture etseydik dangling reference → UB.
>
> `this` neden capture ediliyor? Lambda içinde `config_sub_` member'ına atama yapıyoruz; `late_sub_setup_timer_->cancel()` çağırıyoruz. Bunlar class member; `this` olmadan erişilemez.

> **`cancel()` mi `reset()` mi?** `cancel()` timer'ı durdurur; object yaşamaya devam eder ama bir daha tetiklenmez. `reset()` (shared_ptr method'u) shared_ptr'ı boşaltır → timer object destruction yoluna girer. Burada `cancel` doğru — basit bir "bir daha çağırma" niyeti.

### 3.9 Mismatch subscriber (106–111)

```cpp
auto mismatch_qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable();

mismatch_sub_ = create_subscription<StringMsg>("sensor", mismatch_qos,
    [this](const StringMsg::SharedPtr) { mismatch_rx_count_++; });
```

- `sensor` topic publisher BestEffort. Bu sub Reliable istiyor. Compatibility kuralı bozulur (`BestEffort < Reliable`).
- DDS discovery iki tarafa da uyarı basar:
  - "New publisher discovered on topic '/sensor', offering incompatible QoS." (Reliable sub tarafından)
  - "New subscription discovered on topic '/sensor', requesting incompatible QoS." (BestEffort pub tarafından)
- Sonuç: `mismatch_rx_count_` sonsuza kadar 0.

### 3.10 Stats timer (113–118)

```cpp
stats_timer_ = create_wall_timer(2s, [this]() {
  RCLCPP_INFO(get_logger(),
    "tx[sensor=%zu cmd=%zu]  rx[sensor=%zu cmd=%zu mismatch=%zu]",
    sensor_tx_count_, command_tx_count_,
    sensor_rx_count_, command_rx_count_, mismatch_rx_count_);
});
```

- 2 saniyede bir özet rapor. `%zu` = `size_t` printf format (platforma göre 32/64-bit).
- `[this]` capture yeterli — tüm değişkenler member.

### 3.11 Private üyeler (121–144)

```cpp
private:
  rclcpp::Publisher<StringMsg>::SharedPtr sensor_pub_;
  // ...
  size_t sensor_tx_count_ = 0;
```

- Publisher/Subscription/Timer hepsi `SharedPtr` — ROS2 executor da onlara referans tutar, ortak sahiplik.
- `size_t` — `unsigned int` benzeri, platforma göre 32 veya 64 bit. Sayaçlar için idiomatik.
- `= 0` — **in-class member initializer**. Constructor'da ayrıca atamaya gerek yok.

### 3.12 `main` (147–153)

```cpp
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<QosDemo>());
  rclcpp::shutdown();
  return 0;
}
```

- `rclcpp::init` — ROS2 client library'i kur (RMW initialize, signal handler kur, vs.). Komut satırı argümanlarını da işler (`__node:=...`, `--ros-args ...`).
- `std::make_shared<QosDemo>()` — heap'te `QosDemo` yarat, `shared_ptr<QosDemo>` döndür. (Stack'te yaratırsan `spin` argüman olarak shared_ptr ister; tip uyuşmaz.)
- `rclcpp::spin(node)` — **bloklu çağrı**. Default single-threaded executor'a node'u verir, callback'leri çağırmaya başlar. SIGINT (Ctrl-C) gelince döner.
- `rclcpp::shutdown` — temizlik; tekrar `init` edilebilir.

> `int main(int argc, char ** argv)` — C'den miras, standard `main` signature. `argv` C-string array; `char *` pointer'a pointer.

---

## 4. Çalıştırma + beklenen çıktı

```bash
colcon build --packages-select irp_examples
source install/setup.bash
ros2 launch irp_examples qos_demo.launch.py
```

Beklenenler:

- Başlangıçta **iki** uyarı: incompatible QoS, ikisi de RELIABILITY_QOS_POLICY.
- ~3 saniye civarında: `Late subscriber received: 'config#1'`.
- 2 saniye periyotlu stats: `tx[sensor=…] rx[sensor=…sensor_tx, cmd=cmd_tx, mismatch=0]`.

Mismatch sub'ın **mesaj almadığını** görmek QoS uyumsuzluğu kavramının kanıtıdır. BestEffort'un loopback'te kayıpsız çıkması = aynı process içinde DDS shared-memory transport çalışıyor; gerçek network'te drop görünür.

---

## 5. Sık sorulan / kafa karıştıran noktalar

- **`rclcpp::Node`'dan kalıtım almak yerine `rclcpp::Node` instance'ı tutmak olur muydu?** Olur ama idiomatik değil. Kalıtım daha temiz; helper'lara doğrudan erişim (`get_logger`, `create_publisher`...) sağlar.
- **`make_shared` vs `new`?** `make_shared` daima tercih. Tek allocation, exception safety, daha az boilerplate.
- **`SharedPtr`'i `const &` ile parametre alsam?** Subscription callback signature'ı zaten `const SharedPtr` istiyor. Kendi fonksiyonlarında `const std::shared_ptr<T>&` daha verimli (kopya yok, ref-count artışı yok).
- **Timer member yerine local olsa?** Olmaz — local stack'te yaratılırsa scope bittiğinde `shared_ptr` referansı kaybolur, ref-count düşer, timer destroy olur. Member tutuyoruz ki executor süresince yaşasın.
- **Aynı node'da self-pub/self-sub anlamlı mı?** Production'da nadiren. Burada eğitim amaçlı — tek bir process'te tüm akışı görebiliyoruz.

---

## 6. Sonraki adım için kontrol listesi

Bu dokümanı kapattığında şunları **dosyaya bakmadan** söyleyebiliyor olmalısın:

- [ ] `[this]` ile `[this, config_qos]` arasındaki fark **niye** orada o şekilde.
- [ ] `auto sensor_qos = rclcpp::QoS(...).best_effort().durability_volatile()` ifadesindeki **builder pattern**'i tanırım, hangi setter'ların var olduğunu rclcpp'de aramayı bilirim.
- [ ] BestEffort + Reliable kombinasyonlarından **hangileri eşleşir** soruna doğru cevap verebilirim.
- [ ] TransientLocal'in late-join davranışını **iki cümlede** anlatabilirim.
- [ ] `std::make_shared<T>(...)` ne yapar, niye `new T()` yerine bunu kullanırız.
- [ ] `using namespace std::chrono_literals;` neden gerekli, kaldırırsam ne kırılır.
- [ ] `rclcpp::spin(node)` ne yapar, ne zaman döner.

Eksik kalan varsa o satırın bölümüne dön.
