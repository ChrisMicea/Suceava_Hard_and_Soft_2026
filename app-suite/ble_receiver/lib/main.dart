import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:http/http.dart' as http;

// ── GATT UUIDs – matches BLE_connection.h exactly ────────────────────────────
const kServiceUuid         = "45319765-1234-4521-1234-123456789000";
const kVitalsUuid          = "45319765-1234-4521-1234-123456789001"; // notify  – 12 bytes: hr(f) o2(f) conf(f)
const kMotionUuid          = "45319765-1234-4521-1234-123456789002"; // notify  – 24 bytes: ax ay az gx gy gz (floats)
const kTempUuid            = "45319765-1234-4521-1234-123456789003"; // notify  – 4 bytes:  bodyTemp(f)
const kPanicAckUuid        = "45319765-1234-4521-1234-123456789004"; // write   – uint8: 0x01 = ACK
const kPanicUuid           = "45319765-1234-4521-1234-123456789005"; // notify  – uint8: 0=fall 1=button

// ── Telemetry API ─────────────────────────────────────────────────────────────
const kBaseUrl = "http://172.20.100.87:8080";
const kToken   = "esp32_static_token_12345";

final _apiHeaders = {
  "Authorization": "Bearer $kToken",
  "Content-Type":  "application/json",
};

// ── API helpers ───────────────────────────────────────────────────────────────
Future<void> postVitalSigns(double hr, double o2, double conf) async {
  try {
    await http.post(Uri.parse("$kBaseUrl/api/vital-signs"),
        headers: _apiHeaders,
        body: jsonEncode({
          "heartrate":  hr.round(),
          "oxygen":     double.parse(o2.toStringAsFixed(2)),
          "confidence": double.parse(conf.toStringAsFixed(3)),
        }));
  } catch (e) { debugPrint("[API] vital-signs: $e"); }
}

Future<void> postTemperature(double temp) async {
  try {
    await http.post(Uri.parse("$kBaseUrl/api/temperature"),
        headers: _apiHeaders,
        body: jsonEncode({"temperature": double.parse(temp.toStringAsFixed(2))}));
  } catch (e) { debugPrint("[API] temperature: $e"); }
}

Future<void> postMotion(
    double ax, double ay, double az,
    double gx, double gy, double gz) async {
  try {
    // API expects rotXYZ / accXYZ — we map gyro→rot, acc→acc
    await http.post(Uri.parse("$kBaseUrl/api/motion"),
        headers: _apiHeaders,
        body: jsonEncode({
          "rotX": double.parse(gx.toStringAsFixed(4)),
          "rotY": double.parse(gy.toStringAsFixed(4)),
          "rotZ": double.parse(gz.toStringAsFixed(4)),
          "accX": double.parse(ax.toStringAsFixed(4)),
          "accY": double.parse(ay.toStringAsFixed(4)),
          "accZ": double.parse(az.toStringAsFixed(4)),
        }));
  } catch (e) { debugPrint("[API] motion: $e"); }
}

Future<void> postPanicEvent(String eventType) async {
  try {
    await http.post(Uri.parse("$kBaseUrl/api/panic-events"),
        headers: _apiHeaders,
        body: jsonEncode({"eventType": eventType}));
  } catch (e) { debugPrint("[API] panic-events: $e"); }
}

Future<int?> fetchLatestPanicId() async {
  try {
    final res = await http.get(
        Uri.parse("$kBaseUrl/api/panic-events/latest"), headers: _apiHeaders);
    if (res.statusCode == 200) return jsonDecode(res.body)['id'] as int?;
  } catch (e) { debugPrint("[API] latest-panic: $e"); }
  return null;
}

Future<void> acknowledgePanic(int id) async {
  try {
    await http.put(Uri.parse("$kBaseUrl/api/panic-events/$id/acknowledge"),
        headers: _apiHeaders);
  } catch (e) { debugPrint("[API] acknowledge: $e"); }
}

Future<bool> isPanicAcknowledgedByCaregiver(int id) async {
  try {
    final res = await http.get(
        Uri.parse("$kBaseUrl/api/panic-events/$id"), headers: _apiHeaders);
    if (res.statusCode == 200) {
      final data = jsonDecode(res.body);
      return data['acknowledgedBy'] != null || data['acknowledged'] == true;
    }
  } catch (e) { debugPrint("[API] check-acknowledge: $e"); }
  return false;
}


// ── BLE float decoder (little-endian, matches ESP32 memcpy) ──────────────────
double f32(List<int> b, int offset) {
  final buf = Uint8List.fromList(b.sublist(offset, offset + 4));
  return ByteData.sublistView(buf).getFloat32(0, Endian.little);
}

// ── App entry point ───────────────────────────────────────────────────────────
void main() {
  FlutterBluePlus.setLogLevel(LogLevel.warning);
  runApp(const CrutchApp());
}

class CrutchApp extends StatelessWidget {
  const CrutchApp({super.key});
  @override
  Widget build(BuildContext context) => MaterialApp(
    title: 'FORCE Crutch',
    debugShowCheckedModeBanner: false,
    theme: ThemeData.dark().copyWith(
      colorScheme: const ColorScheme.dark(
        primary:   Color(0xFF00E5FF),
        secondary: Color(0xFFFF4081),
      ),
      scaffoldBackgroundColor: const Color(0xFF0A0A0F),
    ),
    home: const HomePage(),
  );
}

// ── Home page ─────────────────────────────────────────────────────────────────
class HomePage extends StatefulWidget {
  const HomePage({super.key});
  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage>
    with SingleTickerProviderStateMixin {

  // ── BLE handles ─────────────────────────────────────────────────────────────
  BluetoothDevice?         _device;
  BluetoothCharacteristic? _panicAckChar;

  bool _isScanning   = false;
  bool _isConnected  = false;
  bool _isConnecting = false;

  // ── Live sensor values ───────────────────────────────────────────────────────
  double _hr   = 0; // bpm (sent as float from ESP32)
  double _o2   = 0; // SpO₂ %
  double _conf = 0; // sensor confidence 0-1
  double _temp = 0; // body temperature °C
  double _ax   = 0; // acceleration X  m/s²
  double _ay   = 0;
  double _az   = 0;
  double _gx   = 0; // gyro X  °/s
  double _gy   = 0;
  double _gz   = 0;

  // ── API throttle timestamps ──────────────────────────────────────────────────
  DateTime _lastVitalPost  = DateTime(2000);
  DateTime _lastTempPost   = DateTime(2000);
  DateTime _lastMotionPost = DateTime(2000);

  // ── Panic alert tracking ─────────────────────────────────────────────────────
  int? _currentPanicId;
  Timer? _panicCheckTimer;

  // ── Upload status banner ─────────────────────────────────────────────────────
  String _status = '';
  Timer? _statusTimer;

  final List<StreamSubscription> _subs = [];
  late AnimationController _pulseCtrl;

  @override
  void initState() {
    super.initState();
    _pulseCtrl = AnimationController(
        vsync: this, duration: const Duration(milliseconds: 900))
      ..repeat(reverse: true);
  }

  @override
  void dispose() {
    _pulseCtrl.dispose();
    _statusTimer?.cancel();
    _panicCheckTimer?.cancel();
    for (final s in _subs) s.cancel();
    super.dispose();
  }

  void _showStatus(String msg) {
    setState(() => _status = msg);
    _statusTimer?.cancel();
    _statusTimer = Timer(const Duration(seconds: 3),
            () { if (mounted) setState(() => _status = ''); });
  }

  // ── Scan ─────────────────────────────────────────────────────────────────────
  Future<void> _startScan() async {
    if (_isScanning) return;
    setState(() => _isScanning = true);

    await FlutterBluePlus.startScan(
      withServices: [Guid(kServiceUuid)],
      timeout: const Duration(seconds: 12),
    );

    _subs.add(FlutterBluePlus.scanResults.listen((results) async {
      if (_isConnecting || _isConnected) return;
      for (final r in results) {
        final name  = r.device.advName.toLowerCase();
        final uuids = r.advertisementData.serviceUuids
            .map((u) => u.toString().toLowerCase()).toList();
        if (name.contains('force') || name.contains('crutch') ||
            uuids.contains(kServiceUuid)) {
          await FlutterBluePlus.stopScan();
          await _connectTo(r.device);
          break;
        }
      }
    }));

    _subs.add(FlutterBluePlus.isScanning
        .listen((s) { if (!s && mounted) setState(() => _isScanning = false); }));
  }

  // ── Connect ──────────────────────────────────────────────────────────────────
  Future<void> _connectTo(BluetoothDevice device) async {
    setState(() { _isConnecting = true; _device = device; });
    try {
      await device.connect(autoConnect: false);

      // Wait for confirmed connected state before attaching disconnect watcher
      await device.connectionState
          .firstWhere((s) => s == BluetoothConnectionState.connected)
          .timeout(const Duration(seconds: 8));

      setState(() { _isConnected = true; _isConnecting = false; });

      _subs.add(device.connectionState.listen((s) {
        if (s == BluetoothConnectionState.disconnected && _isConnected) {
          debugPrint('[BLE] Disconnected');
          if (mounted) setState(() {
            _isConnected = false;
            _hr = _o2 = _conf = _temp = _ax = _ay = _az = _gx = _gy = _gz = 0;
          });
        }
      }));

      await _discoverServices(device);
    } catch (e) {
      debugPrint('Connect error: $e');
      if (mounted) setState(() { _isConnecting = false; _isConnected = false; });
    }
  }

  // ── Discover & subscribe ─────────────────────────────────────────────────────
  Future<void> _discoverServices(BluetoothDevice device) async {
    final services = await device.discoverServices();
    for (final svc in services) {
      if (svc.uuid.toString().toLowerCase() != kServiceUuid) continue;
      for (final c in svc.characteristics) {
        final uuid = c.uuid.toString().toLowerCase();
        switch (uuid) {
          case kVitalsUuid:   await _sub(c, _onVitals);   break;
          case kMotionUuid:   await _sub(c, _onMotion);   break;
          case kTempUuid:     await _sub(c, _onTemp);     break;
          case kPanicUuid:    await _sub(c, _onPanic);    break;
          case kPanicAckUuid: _panicAckChar = c;          break;
        }
      }
    }
    debugPrint('[BLE] Services discovered. Panic ack char: $_panicAckChar');
  }

  Future<void> _sub(BluetoothCharacteristic c,
      void Function(List<int>) h) async {
    await c.setNotifyValue(true);
    _subs.add(c.onValueReceived.listen(h));
  }

  // ── Characteristic handlers ──────────────────────────────────────────────────

  // Vitals: 12 bytes – heartRate(f32) | oxygen(f32) | confidence(f32)
  void _onVitals(List<int> v) {
    if (v.length < 12) return;
    final hr   = f32(v, 0);
    final o2   = f32(v, 4);
    final conf = f32(v, 8);
    setState(() { _hr = hr; _o2 = o2; _conf = conf; });

    final now = DateTime.now();
    if (now.difference(_lastVitalPost).inSeconds >= 1 && hr > 0 && o2 > 0) {
      _lastVitalPost = now;
      postVitalSigns(hr, o2, conf)
          .then((_) => _showStatus('↑ Vitals posted'));
    }
  }

  // Motion: 24 bytes – ax ay az gx gy gz (all f32, little-endian)
  // ESP32 packs: ax/16384, ay/16384, az/16384, gx/131, gy/131, gz/131
  void _onMotion(List<int> v) {
    if (v.length < 24) return;
    final ax = f32(v, 0);  final ay = f32(v, 4);  final az = f32(v, 8);
    final gx = f32(v, 12); final gy = f32(v, 16); final gz = f32(v, 20);
    setState(() { _ax = ax; _ay = ay; _az = az; _gx = gx; _gy = gy; _gz = gz; });

    final now = DateTime.now();
    if (now.difference(_lastMotionPost).inMilliseconds >= 100) {
      _lastMotionPost = now;
      postMotion(ax, ay, az, gx, gy, gz);
    }
  }

  // Temp: 4 bytes – bodyTemp(f32)
  void _onTemp(List<int> v) {
    if (v.length < 4) return;
    final t = f32(v, 0);
    setState(() => _temp = t);

    final now = DateTime.now();
    if (now.difference(_lastTempPost).inSeconds >= 5 && t > 0) {
      _lastTempPost = now;
      postTemperature(t).then((_) => _showStatus('↑ Temp posted'));
    }
  }

  // Panic: 1 byte – 0=FALL 1=PANIC_BUTTON  (per sendPanic: isFall ? 1 : 0 → inverted label)
  void _onPanic(List<int> v) {
    if (v.isEmpty) return;
    final type = v[0] == 1 ? "FALL_DETECTION" : "PANIC_BUTTON";
    postPanicEvent(type).then((_) async {
      _showStatus('🚨 $type → server');
      final id = await fetchLatestPanicId();
      if (id != null) {
        _currentPanicId = id;
        _startPanicAckPolling();
      }
      _showPanicBanner(type);
    });
  }

  void _showPanicBanner(String type) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showMaterialBanner(MaterialBanner(
      backgroundColor: Colors.red[900],
      content: Text('🚨 $type',
          style: const TextStyle(color: Colors.white,
              fontWeight: FontWeight.bold, fontSize: 15)),
      actions: [
        TextButton(
          onPressed: () async {
            ScaffoldMessenger.of(context).clearMaterialBanners();
            _panicCheckTimer?.cancel();
            _panicCheckTimer = null;
            if (_currentPanicId != null) {
              await acknowledgePanic(_currentPanicId!);
              await _writePanicAck();
              _showStatus('✓ Panic acknowledged');
            }
          },
          child: const Text('ACKNOWLEDGE',
              style: TextStyle(color: Colors.white,
                  fontWeight: FontWeight.bold)),
        ),
      ],
    ));
  }

  void _startPanicAckPolling() {
    _panicCheckTimer?.cancel();
    _panicCheckTimer = Timer.periodic(const Duration(seconds: 1), (_) async {
      if (_currentPanicId != null) {
        final acked = await isPanicAcknowledgedByCaregiver(_currentPanicId!);
        if (acked && mounted) {
          _panicCheckTimer?.cancel();
          _panicCheckTimer = null;
          ScaffoldMessenger.of(context).clearMaterialBanners();
          await _writePanicAck();
          _showStatus('✓ Panic acknowledged by caregiver');
        }
      }
    });
  }

  Future<void> _writePanicAck() async {
    if (_panicAckChar == null) return;
    try {
      await _panicAckChar!.write([0x01], withoutResponse: false);
    } catch (e) { debugPrint('[BLE] panicAck write: $e'); }
  }

  Future<void> _disconnect() async {
    await _device?.disconnect();
    if (mounted) setState(() { _isConnected = false; _device = null; });
  }

  // ── Build ─────────────────────────────────────────────────────────────────────
  @override
  Widget build(BuildContext context) => Scaffold(
    backgroundColor: const Color(0xFF0A0A0F),
    appBar: AppBar(
      backgroundColor: Colors.transparent,
      elevation: 0,
      title: Row(children: [
        const Icon(Icons.health_and_safety, color: Color(0xFF00E5FF)),
        const SizedBox(width: 8),
        const Text('FORCE Crutch',
            style: TextStyle(color: Colors.white,
                fontSize: 18, fontWeight: FontWeight.w600)),
      ]),
      actions: [
        if (_status.isNotEmpty)
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 10),
            child: Center(child: Text(_status,
                style: const TextStyle(
                    color: Color(0xFF69F0AE), fontSize: 12))),
          ),
        if (_isConnected)
          IconButton(
            icon: const Icon(Icons.bluetooth_disabled,
                color: Colors.redAccent),
            tooltip: 'Disconnect',
            onPressed: _disconnect,
          ),
      ],
    ),
    body: _isConnected ? _dashboard() : _connectScreen(),
  );

  // ── Connect screen ────────────────────────────────────────────────────────────
  Widget _connectScreen() => Center(
    child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
      AnimatedBuilder(
        animation: _pulseCtrl,
        builder: (_, __) => Icon(Icons.bluetooth_searching, size: 96,
            color: Color.lerp(const Color(0xFF00E5FF),
                Colors.blueGrey[700], _pulseCtrl.value)),
      ),
      const SizedBox(height: 24),
      Text(
        _isConnecting ? 'Connecting to FORCE Crutch…' : 'No device connected',
        style: TextStyle(color: Colors.grey[400], fontSize: 15),
      ),
      const SizedBox(height: 8),
      Text('Looking for "${"FORCE Crutch"}" via BLE',
          style: TextStyle(color: Colors.grey[700], fontSize: 12)),
      const SizedBox(height: 36),
      ElevatedButton.icon(
        style: ElevatedButton.styleFrom(
          backgroundColor: const Color(0xFF00E5FF),
          foregroundColor: Colors.black,
          padding: const EdgeInsets.symmetric(horizontal: 36, vertical: 15),
          shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(30)),
        ),
        icon: (_isScanning || _isConnecting)
            ? const SizedBox(width: 18, height: 18,
            child: CircularProgressIndicator(strokeWidth: 2.5,
                color: Colors.black))
            : const Icon(Icons.search),
        label: Text(
          _isScanning ? 'Scanning…'
              : _isConnecting ? 'Connecting…'
              : 'Scan & Connect',
          style: const TextStyle(fontWeight: FontWeight.bold),
        ),
        onPressed: (_isScanning || _isConnecting) ? null : _startScan,
      ),
    ]),
  );

  // ── Dashboard ─────────────────────────────────────────────────────────────────
  Widget _dashboard() => ListView(
    padding: const EdgeInsets.all(16),
    children: [
      _connectionBadge(),
      const SizedBox(height: 16),

      // Vitals row
      _sectionLabel('VITAL SIGNS'),
      const SizedBox(height: 8),
      Row(children: [
        Expanded(child: _card(Icons.favorite,       'Heart Rate',
            '${_hr.round()}', 'bpm',    const Color(0xFFFF4081))),
        const SizedBox(width: 12),
        Expanded(child: _card(Icons.air,            'SpO₂',
            _o2.toStringAsFixed(1), '%', const Color(0xFF40C4FF))),
      ]),
      const SizedBox(height: 12),
      Row(children: [
        Expanded(child: _card(Icons.thermostat,     'Body Temp',
            _temp.toStringAsFixed(1), '°C', const Color(0xFFFFAB40))),
        const SizedBox(width: 12),
        Expanded(child: _card(Icons.verified,       'Confidence',
            '${(_conf * 100).toStringAsFixed(0)}', '%',
            const Color(0xFF00E5FF))),
      ]),

      const SizedBox(height: 20),
      _sectionLabel('ACCELEROMETER  (g)'),
      const SizedBox(height: 8),
      _motionRow(
        ('X', _ax, const Color(0xFFFF6D6D)),
        ('Y', _ay, const Color(0xFF69F0AE)),
        ('Z', _az, const Color(0xFF40C4FF)),
      ),

      const SizedBox(height: 16),
      _sectionLabel('GYROSCOPE  (°/s)'),
      const SizedBox(height: 8),
      _motionRow(
        ('X', _gx, const Color(0xFFFFAB40)),
        ('Y', _gy, const Color(0xFFCE93D8)),
        ('Z', _gz, const Color(0xFF80CBC4)),
      ),

      const SizedBox(height: 20),
      _serverInfo(),
    ],
  );

  Widget _connectionBadge() => Container(
    padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
    decoration: BoxDecoration(
      color: const Color(0xFF00E5FF).withOpacity(0.08),
      border: Border.all(color: const Color(0xFF00E5FF).withOpacity(0.25)),
      borderRadius: BorderRadius.circular(12),
    ),
    child: Row(children: [
      const Icon(Icons.bluetooth_connected,
          color: Color(0xFF00E5FF), size: 18),
      const SizedBox(width: 8),
      Expanded(child: Text(
        _device?.advName.isNotEmpty == true
            ? _device!.advName : 'FORCE Crutch',
        style: const TextStyle(color: Color(0xFF00E5FF),
            fontWeight: FontWeight.w600),
      )),
      const Icon(Icons.cloud_upload, color: Color(0xFF69F0AE), size: 14),
      const SizedBox(width: 4),
      const Text('Relaying to API',
          style: TextStyle(color: Color(0xFF69F0AE), fontSize: 11)),
      const SizedBox(width: 4),
      const Icon(Icons.circle, color: Color(0xFF69F0AE), size: 7),
    ]),
  );

  Widget _sectionLabel(String t) => Text(t,
      style: TextStyle(color: Colors.grey[600], fontSize: 10,
          letterSpacing: 1.4, fontWeight: FontWeight.w600));

  Widget _card(IconData icon, String label, String value, String unit,
      Color accent) =>
      Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: const Color(0xFF12121A),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: accent.withOpacity(0.18)),
        ),
        child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
          Icon(icon, color: accent, size: 18),
          const SizedBox(height: 10),
          Text(label,
              style: TextStyle(color: Colors.grey[500], fontSize: 11)),
          const SizedBox(height: 3),
          RichText(text: TextSpan(children: [
            TextSpan(text: value,
                style: TextStyle(color: accent, fontSize: 26,
                    fontWeight: FontWeight.bold)),
            TextSpan(text: ' $unit',
                style: TextStyle(color: Colors.grey[600], fontSize: 11)),
          ])),
        ]),
      );

  Widget _motionRow(
      (String, double, Color) x,
      (String, double, Color) y,
      (String, double, Color) z) =>
      Row(children: [
        Expanded(child: _axisCard(x.$1, x.$2, x.$3)),
        const SizedBox(width: 8),
        Expanded(child: _axisCard(y.$1, y.$2, y.$3)),
        const SizedBox(width: 8),
        Expanded(child: _axisCard(z.$1, z.$2, z.$3)),
      ]);

  Widget _axisCard(String axis, double val, Color accent) => Container(
    padding: const EdgeInsets.symmetric(vertical: 14, horizontal: 12),
    decoration: BoxDecoration(
      color: const Color(0xFF12121A),
      borderRadius: BorderRadius.circular(14),
      border: Border.all(color: accent.withOpacity(0.2)),
    ),
    child: Column(children: [
      Text(axis,
          style: TextStyle(color: accent,
              fontWeight: FontWeight.bold, fontSize: 12)),
      const SizedBox(height: 6),
      Text(val.toStringAsFixed(3),
          style: const TextStyle(color: Colors.white70,
              fontSize: 13, fontFamily: 'monospace')),
    ]),
  );

  Widget _serverInfo() => Container(
    padding: const EdgeInsets.all(12),
    decoration: BoxDecoration(
      color: const Color(0xFF0D1117),
      borderRadius: BorderRadius.circular(10),
      border: Border.all(color: Colors.grey[900]!),
    ),
    child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
      Text('API relay', style: TextStyle(color: Colors.grey[600],
          fontSize: 10, letterSpacing: 0.5)),
      const SizedBox(height: 4),
      Text(kBaseUrl,
          style: const TextStyle(color: Color(0xFF00E5FF),
              fontSize: 12, fontFamily: 'monospace')),
      const SizedBox(height: 3),
      Text('Vitals ≤1 s  ·  Temp ≤5 s  ·  Motion ≤100 ms',
          style: TextStyle(color: Colors.grey[700], fontSize: 10)),
    ]),
  );
}