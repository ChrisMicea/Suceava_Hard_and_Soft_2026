import 'dart:async';
import 'package:flutter_background_service/flutter_background_service.dart';
import 'package:flutter_background_service_android/flutter_background_service_android.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'dart:typed_data';

late FlutterLocalNotificationsPlugin _notificationPlugin;
int? _lastAlertId;
bool _alertAcknowledged = false;

Future<void> initializeBackgroundService() async {
  final service = FlutterBackgroundService();

  // Initialize notifications for background
  const AndroidInitializationSettings androidInit =
    AndroidInitializationSettings('@mipmap/ic_launcher');
  const InitializationSettings initSettings = InitializationSettings(
    android: androidInit,
  );
  _notificationPlugin = FlutterLocalNotificationsPlugin();
  await _notificationPlugin.initialize(initSettings);

  // Create notification channels
  final AndroidFlutterLocalNotificationsPlugin? androidImpl =
    _notificationPlugin.resolvePlatformSpecificImplementation<
      AndroidFlutterLocalNotificationsPlugin>();
  if (androidImpl != null) {
    // Silent channel for foreground service notification
    await androidImpl.createNotificationChannel(
      const AndroidNotificationChannel(
        'service_channel',
        'Service Notifications',
        description: 'Silent notifications for background service',
        importance: Importance.low,
        playSound: false,
        enableVibration: false,
      ),
    );

    // Alert channel with sound for actual alerts
    await androidImpl.createNotificationChannel(
      const AndroidNotificationChannel(
        'alert_channel',
        'Alert Notifications',
        description: 'Background alert notifications',
        importance: Importance.max,
        playSound: true,
        enableVibration: true,
      ),
    );
  }

  await service.configure(
    androidConfiguration: AndroidConfiguration(
      onStart: onStart,
      autoStart: true,
      isForegroundMode: true,
      foregroundServiceTypes: [AndroidForegroundType.dataSync],
      notificationChannelId: 'service_channel',
      initialNotificationTitle: 'Telemetry Monitor',
      initialNotificationContent: 'Monitoring for alerts...',
    ),
    iosConfiguration: IosConfiguration(
      autoStart: true,
      onForeground: onStart,
    ),
  );

  service.startService();
}

@pragma('vm:entry-point')
void onStart(ServiceInstance service) {
  // Initialize notifications in background isolate
  _initializeNotificationsInBackground();

  if (service is AndroidServiceInstance) {
    service.on('setAsForeground').listen((event) {
      service.setAsForegroundService();
    });
    service.on('setAsBackground').listen((event) {
      service.setAsBackgroundService();
    });
  }
  service.on('stopService').listen((event) {
    service.stopSelf();
  });

  // Start alert polling
  Timer.periodic(const Duration(seconds: 12), (timer) async {
    try {
      print("Trying to get response\n");
      final response = await http.get(
        Uri.parse('http://172.20.100.87:8080/api/panic-events/latest'),
      ).timeout(const Duration(seconds: 5));

      if (response.statusCode == 200) {
        final alert = jsonDecode(response.body);
        final alertId = alert['id'] as int;
        final status = alert['status'] as String;

        if (status == 'ACTIVE' && alertId != _lastAlertId) {
          _lastAlertId = alertId;
          _alertAcknowledged = false;

          // Show notification
          await _showBackgroundNotification(alert);
        } else if (status == 'ACKNOWLEDGED' && alertId == _lastAlertId) {
          _alertAcknowledged = true;
          _lastAlertId = null;

          // Cancel notification
          await _notificationPlugin.cancel(alertId);
        }
      }
    } catch (e) {
      print('Background service error: $e');
    }
  });
}

Future<void> _showBackgroundNotification(Map<String, dynamic> alert) async {
  final AndroidNotificationDetails androidDetails =
    AndroidNotificationDetails(
      'alert_channel',
      'Alert Notifications',
      channelDescription: 'Background alert notifications',
      importance: Importance.max,
      priority: Priority.max,
      playSound: true,
      enableVibration: true,
      vibrationPattern: Int64List.fromList(const [0, 500, 250, 500]),
      fullScreenIntent: true,
      autoCancel: false,
    );

  final NotificationDetails notificationDetails = NotificationDetails(
    android: androidDetails,
  );

  await _notificationPlugin.show(
    alert['id'],
    'ALERT: ${alert['eventType']}',
    'Panic event detected - ${alert['createdAt']}',
    notificationDetails,
  );
}

void _initializeNotifications() {
  const AndroidInitializationSettings androidInit =
    AndroidInitializationSettings('@mipmap/ic_launcher');
  const InitializationSettings initSettings = InitializationSettings(
    android: androidInit,
  );
  _notificationPlugin.initialize(initSettings);
}

void _initializeNotificationsInBackground() {
  _notificationPlugin = FlutterLocalNotificationsPlugin();
  const AndroidInitializationSettings androidInit =
    AndroidInitializationSettings('@mipmap/ic_launcher');
  const InitializationSettings initSettings = InitializationSettings(
    android: androidInit,
  );
  _notificationPlugin.initialize(initSettings);

  print('Background service notifications initialized');
}
