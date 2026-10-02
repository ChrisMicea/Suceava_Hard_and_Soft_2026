import 'package:flutter/material.dart';
import 'package:data_monitor/services/telemetry_service.dart';

class AlertsScreen extends StatefulWidget {
  final TelemetryService telemetryService;
  final Function(int) onAcknowledge;

  const AlertsScreen({
    super.key,
    required this.telemetryService,
    required this.onAcknowledge,
  });

  @override
  State<AlertsScreen> createState() => _AlertsScreenState();
}

class _AlertsScreenState extends State<AlertsScreen> {
  final ValueNotifier<int?> _alertCount = ValueNotifier(null);
  final ValueNotifier<String?> _error = ValueNotifier(null);

  @override
  void initState() {
    super.initState();
    _loadAlertCount();
  }

  void _loadAlertCount() async {
    try {
      final data = await widget.telemetryService.getAllPanicEvents();
      _alertCount.value = data.length;
      _error.value = null;
    } catch (e) {
      _error.value = e.toString();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        title: const Text('Panic Alerts'),
        backgroundColor: const Color(0xFF1E88E5),
      ),
      body: ValueListenableBuilder<String?>(
        valueListenable: _error,
        builder: (context, error, _) {
          if (error != null) {
            return Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  const Icon(Icons.error_outline, size: 48, color: Colors.red),
                  const SizedBox(height: 16),
                  Text('Error: $error'),
                  const SizedBox(height: 16),
                  ElevatedButton(
                    onPressed: _loadAlertCount,
                    child: const Text('Retry'),
                  ),
                ],
              ),
            );
          }

          return FutureBuilder<List<Map<String, dynamic>>>(
            future: widget.telemetryService.getAllPanicEvents(),
            builder: (context, snapshot) {
              if (snapshot.hasError) {
                return Center(
                  child: Text('Error loading data'),
                );
              }

              final alerts = snapshot.data ?? [];

              // Show boxes immediately with loading indicator
              if (snapshot.connectionState == ConnectionState.waiting && alerts.isEmpty) {
                // Show skeleton loaders
                return ListView.builder(
                  padding: const EdgeInsets.all(16),
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  itemCount: 3,
                  itemBuilder: (context, index) {
                    return Container(
                      margin: const EdgeInsets.only(bottom: 12),
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: Colors.red.shade50,
                        borderRadius: BorderRadius.circular(8),
                        border: Border.all(
                          color: Colors.red.shade300,
                          width: 2,
                        ),
                      ),
                      child: const Center(child: CircularProgressIndicator()),
                    );
                  },
                );
              }

              if (alerts.isEmpty) {
                return Center(
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(Icons.check_circle, size: 80, color: Colors.green.shade400),
                      const SizedBox(height: 16),
                      const Text('No alerts', style: TextStyle(fontSize: 18)),
                      const SizedBox(height: 8),
                      Text(
                        'All systems normal',
                        style: TextStyle(color: Colors.grey.shade600),
                      ),
                    ],
                  ),
                );
              }

              return ListView.builder(
                padding: const EdgeInsets.all(16),
                itemCount: alerts.length,
                itemBuilder: (context, index) {
                  final alert = alerts[index];
                  final isActive = alert['status'] == 'ACTIVE';
                  final eventType = alert['eventType'] ?? 'UNKNOWN';
                  final createdAt = DateTime.parse(alert['createdAt']);

                  return Container(
                    margin: const EdgeInsets.only(bottom: 12),
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      color: isActive ? Colors.red.shade50 : Colors.green.shade50,
                      borderRadius: BorderRadius.circular(8),
                      border: Border.all(
                        color: isActive ? Colors.red.shade300 : Colors.green.shade300,
                        width: 2,
                      ),
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    eventType,
                                    style: TextStyle(
                                      fontSize: 16,
                                      fontWeight: FontWeight.bold,
                                      color: isActive
                                        ? Colors.red.shade900
                                        : Colors.green.shade900,
                                    ),
                                  ),
                                  const SizedBox(height: 4),
                                  Text(
                                    'ID: ${alert['id']}',
                                    style: TextStyle(
                                      fontSize: 12,
                                      color: Colors.grey.shade600,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                            Container(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 12,
                                vertical: 6,
                              ),
                              decoration: BoxDecoration(
                                color: isActive ? Colors.red : Colors.green,
                                borderRadius: BorderRadius.circular(16),
                              ),
                              child: Text(
                                isActive ? 'ACTIVE' : 'ACKNOWLEDGED',
                                style: const TextStyle(
                                  color: Colors.white,
                                  fontSize: 12,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ),
                          ],
                        ),
                        const SizedBox(height: 12),
                        Text(
                          'Created: ${createdAt.toString().substring(0, 19)}',
                          style: TextStyle(
                            fontSize: 12,
                            color: Colors.grey.shade600,
                          ),
                        ),
                        if (alert['acknowledgedAt'] != null)
                          Padding(
                            padding: const EdgeInsets.only(top: 4),
                            child: Text(
                              'Acknowledged: ${alert['acknowledgedAt']}',
                              style: TextStyle(
                                fontSize: 12,
                                color: Colors.grey.shade600,
                              ),
                            ),
                          ),
                        if (isActive)
                          Padding(
                            padding: const EdgeInsets.only(top: 12),
                            child: SizedBox(
                              width: double.infinity,
                              child: ElevatedButton(
                                onPressed: () async {
                                  try {
                                    await widget.telemetryService
                                      .acknowledgeAlert(alert['id']);
                                    widget.onAcknowledge(alert['id']);
                                    _loadAlertCount();
                                    if (mounted) {
                                      ScaffoldMessenger.of(context).showSnackBar(
                                        const SnackBar(
                                          content: Text('Alert acknowledged'),
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
                                },
                                style: ElevatedButton.styleFrom(
                                  backgroundColor: Colors.red.shade700,
                                ),
                                child: const Text('Acknowledge Alert'),
                              ),
                            ),
                          ),
                      ],
                    ),
                  );
                },
              );
            },
          );
        },
      ),
    );
  }
}
