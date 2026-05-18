# ROADMAP

> `industrial-robotics` projesi için phase-by-phase ilerleme takip dökümanı.
> Tam plan ve mimari prensipler: project context / `docs/PLAN.md`.

---

## Status

- **Current phase:** P0 — ROS2 fluency
- **Started:** 2026-05-15
- **Target completion:** Q3 2026 (16–24 hafta, ~15–20 saat/hafta)
- **Pace:** slow-and-sure. Deadline pressure yok.

**Status legend:** 🟢 done · 🟡 in-progress · ⚪ todo · 🔴 blocked

---

## Phase Overview

| #   | Phase                              | Stage        | Status         | Exit criteria (short)                                            |
| --- | ---------------------------------- | ------------ | -------------- | ---------------------------------------------------------------- |
| P0  | ROS2 fluency                       | Foundation   | 🟡 In progress | Sıfırdan yazılmış UR5e xacro rviz'de doğru görünür               |
| P1  | `ros2_control` deep dive           | Foundation   | ⚪ Todo        | Custom hardware_interface + controller plugin Gazebo'da çalışır  |
| P2  | Kinematik sıfırdan                 | Foundation   | ⚪ Todo        | FK + Jacobian + IK; Pinocchio ile sayısal eşleşme                |
| P3  | Joint + Cartesian controllers      | Control      | ⚪ Todo        | Cartesian impedance controller RT loop'ta jitter altında çalışır |
| P4  | MoveIt2 entegrasyonu               | Control      | ⚪ Todo        | 3 planner benchmark + programmatic planning scene                |
| P5  | Perception                         | Integration  | ⚪ Todo        | Hand-eye calibration + AprilTag visual servoing                  |
| P6  | Force-controlled tasks             | Integration  | ⚪ Todo        | Peg-in-hole hybrid force-position controller ile başarılı        |
| P7  | Task layer                         | Integration  | ⚪ Todo        | BT + error injection ile recovery senaryoları                    |
| P8  | Multi-robot generalization         | Production   | ⚪ Todo        | UR5e → Franka swap: sadece description + hardware değişti        |
| P9  | Production hardening               | Production   | ⚪ Todo        | PREEMPT_RT + CI + tests + Foxglove dashboard                     |
| P10 | Capstone                           | Production   | ⚪ Todo        | Multi-step task + tek launch arg ile iki robotta çalışır         |

---

## Stage 1: Foundation (4–6 hafta)

### P0 — ROS2 fluency 🟡

- [x] `irp_msgs` paketi: en az 1 custom message, 1 service, 1 action
- [ ] Lifecycle node örneği (configure → activate → deactivate)
- [ ] Layered launch sistemi (YAML config + Python composition)
- [ ] En az 3 farklı QoS profilinin farkını gösteren mini demo
- [ ] UR5e xacro sıfırdan yazıldı (vendor URDF okundu, kopyalanmadı)
- [ ] rviz'de doğru link/joint hiyerarşisi görünüyor
- [ ] Anti-pattern check: hiçbir dosya tutorial'dan birebir kopyalanmadı

**Mimari kararlar (P0'da netleşmesi gerekenler):**
- Xacro parçalama granularity'si
- Vendor-specific parametrelerin enjeksiyon yolu (P8'de Franka swap'i temiz olsun)
- ros2_control tag'lerinin URDF içi mi yan dosya mı

### P1 — `ros2_control` deep dive ⚪

- [ ] Mock `hardware_interface` plugin (sadece state echo)
- [ ] `gz_ros2_control` ile Gazebo bağlantısı
- [ ] Custom controller plugin (en basit: pass-through forward controller)
- [ ] Controller chain configure ve runtime switching
- [ ] **Okuma:** `ResourceManager`, `ControllerManager`, `ControllerInterface` başlıkları
- [ ] **Okuma:** `ur_robot_driver` hardware plugin yapısı

### P2 — Kinematik sıfırdan ⚪

- [ ] UR5e DH parametreleri (vendor datasheet'ten teyit)
- [ ] Forward Kinematics (C++ veya Python)
- [ ] Geometric Jacobian
- [ ] Damped least-squares IK
- [ ] Pinocchio ile numerical eşleşme test'i (unit test)
- [ ] **Okuma:** Modern Robotics (Lynch & Park) Ch 3–6

---

## Stage 2: Control (3–5 hafta)

### P3 — Joint ve Cartesian controllers ⚪

- [ ] Custom `joint_trajectory_controller` (kendi yazımı)
- [ ] Cartesian velocity controller (Jacobian-based, singularity-aware damping)
- [ ] **Cartesian impedance controller** (en kritik)
- [ ] RT loop discipline: no alloc, no dynamic dispatch in hot path
- [ ] Jitter ölçümü ve raporu

### P4 — MoveIt2 entegrasyonu ⚪

- [ ] Tüm config dosyaları el yazımı (SRDF, kinematics.yaml, OMPL planner config, controllers.yaml)
- [ ] Programmatic planning scene populating interface
- [ ] Planner benchmark: RRTConnect vs RRTstar vs CHOMP (success rate, path length, time)
- [ ] MoveIt2 → kendi controller stack'ine clean handoff

---

## Stage 3: Integration (5–7 hafta)

### P5 — Perception ⚪

- [ ] Gazebo kamera plugin entegrasyonu
- [ ] Intrinsic calibration (OpenCV manuel, chessboard veya ChArUco)
- [ ] **Hand-eye calibration** (kritik milestone)
- [ ] AprilTag-based pose estimation
- [ ] Basit visual servoing demo (göz → kol)
- [ ] tf2 frame naming discipline doc

### P6 — Force-controlled tasks ⚪

- [ ] Simüle F/T sensörü (noise + drift modelleme)
- [ ] Hybrid force-position controller (kendi elimle, MoveIt'in hazır olanı değil)
- [ ] Peg-in-hole başarı: belirli tolerans + tekrarlanabilirlik
- [ ] AIC task'inin "temiz" versiyonu, deadline'sız

### P7 — Task layer ⚪

- [ ] BehaviorTree.CPP v4 ile pick-place tree
- [ ] Aynı task'in FSM alternatifi (kıyas için)
- [ ] Error injection harness (sensor drop, action fail, collision detect)
- [ ] Recovery behaviors: retry, fallback, safe-abort

---

## Stage 4: Production (4–6 hafta)

### P8 — Multi-robot generalization ⚪

- [ ] Franka Panda xacro `irp_description` altında
- [ ] Franka hardware adapter `irp_hardware/franka_adapter/` altında
- [ ] Aynı task tree iki robotta çalışıyor (launch arg ile swap)
- [ ] **Audit:** P8 sonu — değişen dosyaların listesi. İdeal: yalnızca `irp_description` + `irp_hardware`.

### P9 — Production hardening ⚪

- [ ] PREEMPT_RT kernel kurulumu + cyclictest baseline
- [ ] launch_testing ile integration test suite
- [ ] gtest (C++) + pytest (Python) unit test coverage
- [ ] GitHub Actions CI: build + test on push
- [ ] Foxglove Studio dashboard (RT plots + node graph)

### P10 — Capstone ⚪

- [ ] Multi-step task: lift → place → insert
- [ ] Tek launch arg ile UR5e ↔ Franka swap çalışıyor
- [ ] README finalize (proje amacı + quick start + architecture diagram)
- [ ] `docs/DESIGN.md` finalize
- [ ] Package dependency graph (otomatik üretim)
- [ ] Demo video

---

## Progress Log

| Tarih      | Faz | Milestone                          | Notlar                                           |
| ---------- | --- | ---------------------------------- | ------------------------------------------------ |
| 2026-05-15 | P0  | Environment + repo skeleton        | Ubuntu 24.04, ROS2 Jazzy, Gazebo Harmonic kurulu |
| 2026-05-15 | P0  | `irp_msgs` package                 | RobotMode.msg + SetRobotMode.srv + MoveToHome.action; rosidl pipeline kavrandı |

---

## Notes for Future Self

- Bir fazın "Exit criteria" tamamlandığında bu dosyayı güncelle, status 🟢, tarih ve kısa not Progress Log'a.
- Mimari kararlar bu dosyada **tutulmaz**, `docs/PLAN.md` veya ayrı `docs/decisions/` (ADR) altında yaşar.
- Plan değişirse: ROADMAP güncellenir, **plan değişikliğinin gerekçesi** Progress Log'a yazılır.