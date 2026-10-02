import 'dart:async';

import 'package:flutter/material.dart';
import 'package:data_monitor/services/telemetry_service.dart';
import 'package:data_monitor/services/notification_service.dart';
import 'package:data_monitor/models/telemetry_models.dart';
import 'package:data_monitor/widgets/data_card.dart';
import 'vital_signs_screen.dart';
import 'temperature_screen.dart';
import 'motion_screen.dart';
import 'alerts_screen.dart';

class HomeScreen extends StatefulWidget {
  final TelemetryService telemetryService;
  final bool hasActiveAlert;
  final Function(int) onAcknowledgeAlert;

  const HomeScreen({
    super.key,
    required this.telemetryService,
    required this.hasActiveAlert,
    required this.onAcknowledgeAlert,
  });

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final ValueNotifier<VitalSigns?> _vitalSigns = ValueNotifier(null);
  final ValueNotifier<Temperature?> _temperature = ValueNotifier(null);
  final ValueNotifier<Motion?> _motion = ValueNotifier(null);
  final ValueNotifier<List<Motion>> _motionReadings = ValueNotifier([]);
  final ValueNotifier<bool> _vitalSignsLoading = ValueNotifier(false);
  final ValueNotifier<bool> _temperatureLoading = ValueNotifier(false);
  final ValueNotifier<bool> _motionLoading = ValueNotifier(false);
  final ValueNotifier<String?> _error = ValueNotifier(null);
  final ValueNotifier<bool> _hasAlert = ValueNotifier(false);
  final ValueNotifier<int?> _alertId = ValueNotifier(null);
  Timer? _dataTimer;
  Timer? _alertTimer;

  @override
  void initState() {
    super.initState();
    _loadVitalSigns();
    _loadTemperature();
    _loadMotion();
    _startDataPolling();
    _startAlertPolling();
  }

  void _loadVitalSigns() async {
    try {
      _vitalSignsLoading.value = true;
      final vitalSigns = await widget.telemetryService.getLatestVitalSigns();
      _vitalSigns.value = vitalSigns;
      _error.value = null;
      _vitalSignsLoading.value = false;
    } catch (e) {
      _error.value = e.toString();
      _vitalSignsLoading.value = false;
    }
  }

  void _loadTemperature() async {
    try {
      _temperatureLoading.value = true;
      final temperature = await widget.telemetryService.getLatestTemperature();
      _temperature.value = temperature;
      _error.value = null;
      _temperatureLoading.value = false;
    } catch (e) {
      _error.value = e.toString();
      _temperatureLoading.value = false;
    }
  }

  void _loadMotion() async {
    try {
      _motionLoading.value = true;
      final motion = await widget.telemetryService.getLatestMotion();
      final motionReadings = await widget.telemetryService.getTodaysMotion();
      _motion.value = motion;
      _motionReadings.value = motionReadings;
      _error.value = null;
      _motionLoading.value = false;
    } catch (e) {
      _error.value = e.toString();
      _motionLoading.value = false;
    }
  }

  void _loadData() async {
    _loadVitalSigns();
    _loadTemperature();
    _loadMotion();
  }

  void _startDataPolling() {
    _dataTimer = Timer.periodic(const Duration(seconds: 10), (_) {
      _loadData();
    });
  }

  void _startAlertPolling() {
    _alertTimer = Timer.periodic(const Duration(seconds: 8), (_) async {
      try {
        final alert = await widget.telemetryService.getLatestPanicEvent();
        if (alert != null && alert['status'] == 'ACTIVE') {
          _hasAlert.value = true;
          _alertId.value = alert['id'];
        } else {
          _hasAlert.value = false;
          _alertId.value = null;
        }
      } catch (e) {
        debugPrint('Error polling alerts: $e');
      }
    });
  }

  void _acknowledgeAlert() async {
    if (_alertId.value != null) {
      try {
        await widget.telemetryService.acknowledgeAlert(_alertId.value!);
        await NotificationService().cancelNotification(_alertId.value!);
        _hasAlert.value = false;
        _alertId.value = null;
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('Alert acknowledged'),
              duration: Duration(seconds: 2),
            ),
          );
        }
      } catch (e) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text('Error: $e')),
          );
        }
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        title: const Text('Telemetry Monitor'),
        backgroundColor: const Color(0xFF1E88E5),
        elevation: 0,
        actions: [
          ValueListenableBuilder<bool>(
            valueListenable: _hasAlert,
            builder: (context, hasAlert, _) {
              if (hasAlert) {
                return Padding(
                  padding: const EdgeInsets.all(16),
                  child: Center(
                    child: Container(
                      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                      decoration: BoxDecoration(
                        color: Colors.white,
                        borderRadius: BorderRadius.circular(20),
                      ),
                      child: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const Icon(Icons.warning, color: Colors.red, size: 18),
                          const SizedBox(width: 6),
                          const Text(
                            'ALERT',
                            style: TextStyle(
                              color: Colors.red,
                              fontWeight: FontWeight.bold,
                              fontSize: 12,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                );
              }
              return IconButton(
                icon: const Icon(Icons.notifications),
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (context) => AlertsScreen(
                        telemetryService: widget.telemetryService,
                        onAcknowledge: (id) {
                          _acknowledgeAlert();
                        },
                      ),
                    ),
                  );
                },
              );
            },
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            ValueListenableBuilder<bool>(
              valueListenable: _hasAlert,
              builder: (context, hasAlert, _) {
                if (hasAlert) {
                  return Column(
                    children: [
                      Container(
                        margin: const EdgeInsets.only(bottom: 16),
                        padding: const EdgeInsets.all(16),
                        decoration: BoxDecoration(
                          color: Colors.red.shade100,
                          border: Border.all(
                            color: Colors.red.shade300,
                            width: 2,
                          ),
                          borderRadius: BorderRadius.circular(8),
                        ),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Row(
                              children: [
                                Icon(
                                  Icons.warning,
                                  color: Colors.red.shade700,
                                  size: 24,
                                ),
                                const SizedBox(width: 12),
                                Expanded(
                                  child: Text(
                                    'Active Alert Detected!',
                                    style: TextStyle(
                                      color: Colors.red.shade900,
                                      fontWeight: FontWeight.w600,
                                      fontSize: 16,
                                    ),
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 12),
                            SizedBox(
                              width: double.infinity,
                              child: ElevatedButton(
                                onPressed: _acknowledgeAlert,
                                style: ElevatedButton.styleFrom(
                                  backgroundColor: Colors.red.shade700,
                                  foregroundColor: Colors.white,
                                ),
                                child: const Text('Acknowledge Alert'),
                              ),
                            ),
                          ],
                        ),
                      ),
                    ],
                  );
                }
                return const SizedBox.shrink();
              },
            ),
            Text(
              'Current Status',
              style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                color: const Color(0xFF0D47A1),
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),
            GridView.count(
              crossAxisCount: 1,
              crossAxisSpacing: 12,
              mainAxisSpacing: 12,
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              childAspectRatio: 2.1
              ,
              children: [
                ValueListenableBuilder<bool>(
                  valueListenable: _vitalSignsLoading,
                  builder: (context, isLoading, _) {
                    return ValueListenableBuilder<VitalSigns?>(
                      valueListenable: _vitalSigns,
                      builder: (context, vital, _) {
                        if (isLoading && vital == null) {
                          return Container(
                            decoration: BoxDecoration(
                              color: const Color(0xFFE3F2FD),
                              borderRadius: BorderRadius.circular(12),
                              border: Border.all(
                                color: const Color(0xFF64B5F6),
                              ),
                            ),
                            child: const Center(child: CircularProgressIndicator()),
                          );
                        }
                        return DataCard(
                          title: 'Heart Rate',
                          icon: '❤️',
                          value: vital?.heartrate.toString() ?? 'N/A',
                          unit: 'bpm',
                          isAlert: false,
                          onTap: () {
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (context) => VitalSignsScreen(
                                  telemetryService: widget.telemetryService,
                                  isAlert: false,
                                ),
                              ),
                            );
                          },
                        );
                      },
                    );
                  },
                ),
                ValueListenableBuilder<bool>(
                  valueListenable: _vitalSignsLoading,
                  builder: (context, isLoading, _) {
                    return ValueListenableBuilder<VitalSigns?>(
                      valueListenable: _vitalSigns,
                      builder: (context, vital, _) {
                        if (isLoading && vital == null) {
                          return Container(
                            decoration: BoxDecoration(
                              color: const Color(0xFFE3F2FD),
                              borderRadius: BorderRadius.circular(12),
                              border: Border.all(
                                color: const Color(0xFF64B5F6),
                              ),
                            ),
                            child: const Center(child: CircularProgressIndicator()),
                          );
                        }
                        return DataCard(
                          title: 'Oxygen Level',
                          icon: '💨',
                          value: vital?.oxygen.toStringAsFixed(1) ?? 'N/A',
                          unit: '%',
                          isAlert: false,
                          onTap: () {
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (context) => VitalSignsScreen(
                                  telemetryService: widget.telemetryService,
                                  isAlert: false,
                                ),
                              ),
                            );
                          },
                        );
                      },
                    );
                  },
                ),
                ValueListenableBuilder<bool>(
                  valueListenable: _temperatureLoading,
                  builder: (context, isLoading, _) {
                    return ValueListenableBuilder<Temperature?>(
                      valueListenable: _temperature,
                      builder: (context, temp, _) {
                        if (isLoading && temp == null) {
                          return Container(
                            decoration: BoxDecoration(
                              color: const Color(0xFFE3F2FD),
                              borderRadius: BorderRadius.circular(12),
                              border: Border.all(
                                color: const Color(0xFF64B5F6),
                              ),
                            ),
                            child: const Center(child: CircularProgressIndicator()),
                          );
                        }
                        return DataCard(
                          title: 'Temperature',
                          icon: '🌡️',
                          value: temp?.temperature.toStringAsFixed(1) ?? 'N/A',
                          unit: '°C',
                          isAlert: false,
                          onTap: () {
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (context) => TemperatureScreen(
                                  telemetryService: widget.telemetryService,
                                  isAlert: false,
                                ),
                              ),
                            );
                          },
                        );
                      },
                    );
                  },
                ),
                ValueListenableBuilder<bool>(
                  valueListenable: _motionLoading,
                  builder: (context, isLoading, _) {
                    return ValueListenableBuilder<List<Motion>>(
                      valueListenable: _motionReadings,
                      builder: (context, motionReadings, _) {
                        if (isLoading && motionReadings.isEmpty) {
                          return Container(
                            decoration: BoxDecoration(
                              color: const Color(0xFFE3F2FD),
                              borderRadius: BorderRadius.circular(12),
                              border: Border.all(
                                color: const Color(0xFF64B5F6),
                              ),
                            ),
                            child: const Center(child: CircularProgressIndicator()),
                          );
                        }
                        return DataCard(
                          title: 'Motion',
                          icon: '📍',
                          value: motionReadings.length.toString(),
                          unit: 'reading${motionReadings.length != 1 ? 's' : ''}',
                          isAlert: false,
                          onTap: () {
                            Navigator.push(
                              context,
                              MaterialPageRoute(
                                builder: (context) => MotionScreen(
                                  telemetryService: widget.telemetryService,
                                  isAlert: false,
                                ),
                              ),
                            );
                          },
                        );
                      },
                    );
                  },
                ),
              ],
            ),
            const SizedBox(height: 24),
            Center(
              child: ElevatedButton.icon(
                onPressed: _loadData,
                icon: const Icon(Icons.refresh),
                label: const Text('Refresh'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color(0xFF1E88E5),
                ),
              ),
            ),
            const SizedBox(height: 16),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    _dataTimer?.cancel();
    _alertTimer?.cancel();
    _vitalSigns.dispose();
    _temperature.dispose();
    _motion.dispose();
    _motionReadings.dispose();
    _vitalSignsLoading.dispose();
    _temperatureLoading.dispose();
    _motionLoading.dispose();
    _error.dispose();
    _hasAlert.dispose();
    _alertId.dispose();
    super.dispose();
  }
}

