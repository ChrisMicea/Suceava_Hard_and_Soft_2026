import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'dart:core';
import 'dart:typed_data';
import 'package:flutter/material.dart';

class NotificationService {
  static final NotificationService _instance = NotificationService._internal();
  static final FlutterLocalNotificationsPlugin _flutterLocalNotificationsPlugin =
    FlutterLocalNotificationsPlugin();

  factory NotificationService() {
    return _instance;
  }

  NotificationService._internal();

  Future<void> initialize() async {
    const AndroidInitializationSettings initializationSettingsAndroid =
      AndroidInitializationSettings('@mipmap/ic_launcher');

    const InitializationSettings initializationSettings = InitializationSettings(
      android: initializationSettingsAndroid,
    );

    await _flutterLocalNotificationsPlugin.initialize(
      initializationSettings,
    );

    // Create notification channel with sound
    await _createNotificationChannel();
  }

  Future<void> _createNotificationChannel() async {
    final AndroidFlutterLocalNotificationsPlugin? androidImplementation =
      _flutterLocalNotificationsPlugin.resolvePlatformSpecificImplementation<
        AndroidFlutterLocalNotificationsPlugin>();

    if (androidImplementation != null) {
      await androidImplementation.createNotificationChannel(
        const AndroidNotificationChannel(
          'alert_channel',
          'Alert Notifications',
          description: 'Notifications for telemetry alerts',
          importance: Importance.max,
          playSound: true,
          enableVibration: true,
        ),
      );
    }
  }

  Future<void> showAlertNotification({
    required String title,
    required String body,
    required int alertId,
  }) async {
    final AndroidNotificationDetails androidNotificationDetails =
      AndroidNotificationDetails(
        'alert_channel',
        'Alert Notifications',
        channelDescription: 'Notifications for telemetry alerts',
        importance: Importance.max,
        priority: Priority.max,
        playSound: true,
        enableVibration: true,
        vibrationPattern: Int64List.fromList(const [0, 500, 250, 500]),
        fullScreenIntent: true,
        autoCancel: false,
      );

    final NotificationDetails notificationDetails = NotificationDetails(
      android: androidNotificationDetails,
    );

    await _flutterLocalNotificationsPlugin.show(
      alertId,
      title,
      body,
      notificationDetails,
    );
  }

  Future<void> cancelNotification(int alertId) async {
    await _flutterLocalNotificationsPlugin.cancel(alertId);
  }
}

