import 'dart:async';

import 'package:flutter/material.dart';
import 'package:data_monitor/screens/home_screen.dart';
import 'package:data_monitor/services/telemetry_service.dart';
import 'package:data_monitor/services/notification_service.dart';
import 'package:data_monitor/services/background_service.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await NotificationService().initialize();
  await initializeBackgroundService();
  runApp(const MyApp());
}

class AlertState extends ChangeNotifier {
  int? _activeAlertId;
  bool _hasActiveAlert = false;

  bool get hasActiveAlert => _hasActiveAlert;
  int? get activeAlertId => _activeAlertId;

  void setAlert(int? alertId, bool isActive) {
    if (_activeAlertId != alertId || _hasActiveAlert != isActive) {
      _activeAlertId = alertId;
      _hasActiveAlert = isActive;
      notifyListeners();
    }
  }

  void clearAlert() {
    _activeAlertId = null;
    _hasActiveAlert = false;
    notifyListeners();
  }
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  late TelemetryService _telemetryService;
  late AlertState _alertState;
  Timer? _alertTimer;

  @override
  void initState() {
    super.initState();
    _telemetryService = TelemetryService();
    _alertState = AlertState();
    _startAlertPolling();
  }

  void _startAlertPolling() {
    _alertTimer = Timer.periodic(const Duration(seconds: 3), (_) async {
      try {
        final alert = await _telemetryService.getLatestPanicEvent();
        if (alert != null && alert['status'] == 'ACTIVE') {
          if (!_alertState.hasActiveAlert) {
            _alertState.setAlert(alert['id'], true);
            // Show notification with sound
            await NotificationService().showAlertNotification(
              title: 'ALERT: ${alert['eventType']}',
              body: 'Active alert detected on the device',
              alertId: alert['id'],
            );
          }
        } else {
          _alertState.clearAlert();
        }
      } catch (e) {
        debugPrint('Error polling alerts: $e');
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: _alertState,
      builder: (context, _) {
        return MaterialApp(
          debugShowCheckedModeBanner: false,
          theme: ThemeData(
            useMaterial3: true,
            colorScheme: ColorScheme.fromSeed(
              seedColor: const Color(0xFF1E88E5),
              brightness: Brightness.light,
            ),
            scaffoldBackgroundColor: _alertState.hasActiveAlert
              ? const Color(0xFFFCECEC)
              : Colors.white,
          ),
          home: HomeScreen(
            telemetryService: _telemetryService,
            hasActiveAlert: _alertState.hasActiveAlert,
            onAcknowledgeAlert: (_) => _alertState.clearAlert(),
          ),
        );
      },
    );
  }

  @override
  void dispose() {
    _alertTimer?.cancel();
    _alertState.dispose();
    super.dispose();
  }
}