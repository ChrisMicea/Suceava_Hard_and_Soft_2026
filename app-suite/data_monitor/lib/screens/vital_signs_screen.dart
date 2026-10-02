import 'package:flutter/material.dart';
import 'package:data_monitor/services/telemetry_service.dart';
import 'package:data_monitor/models/telemetry_models.dart';
import 'package:fl_chart/fl_chart.dart';

class VitalSignsScreen extends StatefulWidget {
  final TelemetryService telemetryService;
  final bool isAlert;

  const VitalSignsScreen({
    super.key,
    required this.telemetryService,
    required this.isAlert,
  });

  @override
  State<VitalSignsScreen> createState() => _VitalSignsScreenState();
}

class _VitalSignsScreenState extends State<VitalSignsScreen> {
  final ValueNotifier<int?> _recordCount = ValueNotifier(null);
  final ValueNotifier<String?> _error = ValueNotifier(null);

  @override
  void initState() {
    super.initState();
    _loadRecordCount();
  }

  void _loadRecordCount() async {
    try {
      final data = await widget.telemetryService.getTodaysVitalSigns();
      _recordCount.value = data.length;
      _error.value = null;
    } catch (e) {
      _error.value = e.toString();
    }
  }

  @override
  Widget build(BuildContext context) {
    final bgColor = widget.isAlert
      ? const Color(0xFFFCECEC)
      : Colors.white;

    return Scaffold(
      backgroundColor: bgColor,
      appBar: AppBar(
        title: const Text('Vital Signs'),
        backgroundColor: widget.isAlert
          ? Colors.red.shade400
          : const Color(0xFF1E88E5),
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
                    onPressed: _loadRecordCount,
                    child: const Text('Retry'),
                  ),
                ],
              ),
            );
          }

          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                FutureBuilder<List<VitalSigns>>(
                  future: widget.telemetryService.getVitalSigns(),
                  builder: (context, chartSnapshot) {
                    final vitalSigns = chartSnapshot.data ?? [];
                    if (vitalSigns.isNotEmpty) {
                      return Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            'Heart Rate & Oxygen Trend',
                            style: Theme.of(context).textTheme.titleMedium?.copyWith(
                              color: widget.isAlert
                                ? Colors.red.shade900
                                : const Color(0xFF0D47A1),
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                          const SizedBox(height: 12),
                          Container(
                            height: 300,
                            padding: const EdgeInsets.all(12),
                            decoration: BoxDecoration(
                              color: widget.isAlert
                                ? Colors.red.shade50
                                : const Color(0xFFE3F2FD),
                              borderRadius: BorderRadius.circular(8),
                              border: Border.all(
                                color: widget.isAlert
                                  ? Colors.red.shade300
                                  : const Color(0xFF64B5F6),
                              ),
                            ),
                            child: LineChart(
                              LineChartData(
                                gridData: FlGridData(show: true, drawVerticalLine: false),
                                titlesData: FlTitlesData(
                                  topTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
                                  rightTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
                                  leftTitles: AxisTitles(
                                    sideTitles: SideTitles(
                                      showTitles: true,
                                      reservedSize: 40,
                                      getTitlesWidget: (value, meta) {
                                        return Text(value.toInt().toString(), style: const TextStyle(fontSize: 10));
                                      },
                                    ),
                                  ),
                                  bottomTitles: AxisTitles(
                                    sideTitles: SideTitles(
                                      showTitles: true,
                                      getTitlesWidget: (value, meta) {
                                        final index = value.toInt();
                                        if (index < 0 || index >= vitalSigns.length) return const Text('');
                                        final time = vitalSigns[index].createdAt;
                                        return Text('${time.hour}:${time.minute.toString().padLeft(2, '0')}',
                                          style: const TextStyle(fontSize: 8));
                                      },
                                    ),
                                  ),
                                ),
                                lineBarsData: [
                                  LineChartBarData(
                                    spots: vitalSigns.asMap().entries.map((e) {
                                      return FlSpot(e.key.toDouble(), e.value.heartrate.toDouble());
                                    }).toList(),
                                    isCurved: true,
                                    color: Colors.red,
                                    barWidth: 2,
                                    dotData: const FlDotData(show: false),
                                  ),
                                  LineChartBarData(
                                    spots: vitalSigns.asMap().entries.map((e) {
                                      return FlSpot(e.key.toDouble(), e.value.oxygen);
                                    }).toList(),
                                    isCurved: true,
                                    color: Colors.blue,
                                    barWidth: 2,
                                    dotData: const FlDotData(show: false),
                                  ),
                                ],
                              ),
                            ),
                          ),
                          const SizedBox(height: 12),
                          Row(
                            children: [
                              Row(
                                children: [
                                  Container(width: 12, height: 12, color: Colors.red),
                                  const SizedBox(width: 6),
                                  const Text('Heart Rate (bpm)', style: TextStyle(fontSize: 12)),
                                ],
                              ),
                              const SizedBox(width: 20),
                              Row(
                                children: [
                                  Container(width: 12, height: 12, color: Colors.blue),
                                  const SizedBox(width: 6),
                                  const Text('Oxygen (%)', style: TextStyle(fontSize: 12)),
                                ],
                              ),
                            ],
                          ),
                          const SizedBox(height: 24),
                        ],
                      );
                    }
                    return const SizedBox.shrink();
                  },
                ),
                ValueListenableBuilder<int?>(
                  valueListenable: _recordCount,
                  builder: (context, count, _) {
                    return Text(
                      'Today\'s History (${count ?? '...'} records)',
                      style: Theme.of(context).textTheme.titleLarge?.copyWith(
                        color: widget.isAlert
                          ? Colors.red.shade900
                          : const Color(0xFF0D47A1),
                      ),
                    );
                  },
                ),
                const SizedBox(height: 16),
                FutureBuilder<List<VitalSigns>>(
                  future: widget.telemetryService.getTodaysVitalSigns(),
                  builder: (context, snapshot) {
                    if (snapshot.hasError) {
                      return Center(
                        child: Text('Error loading data'),
                      );
                    }

                    final vitalSigns = snapshot.data ?? [];

                    // Show boxes immediately with loading indicator
                    if (snapshot.connectionState == ConnectionState.waiting && vitalSigns.isEmpty) {
                      // Show skeleton loaders
                      return ListView.builder(
                        shrinkWrap: true,
                        physics: const NeverScrollableScrollPhysics(),
                        itemCount: 3,
                        itemBuilder: (context, index) {
                          return Container(
                            margin: const EdgeInsets.only(bottom: 12),
                            padding: const EdgeInsets.all(16),
                            decoration: BoxDecoration(
                              color: widget.isAlert
                                ? Colors.red.shade50
                                : const Color(0xFFE3F2FD),
                              borderRadius: BorderRadius.circular(8),
                              border: Border.all(
                                color: widget.isAlert
                                  ? Colors.red.shade300
                                  : const Color(0xFF64B5F6),
                              ),
                            ),
                            child: const Center(child: CircularProgressIndicator()),
                          );
                        },
                      );
                    }

                    return ListView.builder(
                      shrinkWrap: true,
                      physics: const NeverScrollableScrollPhysics(),
                      itemCount: vitalSigns.length,
                      itemBuilder: (context, index) {
                        final vital = vitalSigns[index];
                        return Container(
                          margin: const EdgeInsets.only(bottom: 12),
                          padding: const EdgeInsets.all(16),
                          decoration: BoxDecoration(
                            color: widget.isAlert
                              ? Colors.red.shade50
                              : const Color(0xFFE3F2FD),
                            borderRadius: BorderRadius.circular(8),
                            border: Border.all(
                              color: widget.isAlert
                                ? Colors.red.shade300
                                : const Color(0xFF64B5F6),
                            ),
                          ),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Row(
                                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                children: [
                                  Text(
                                    'Record #${vital.id}',
                                    style: TextStyle(
                                      fontWeight: FontWeight.bold,
                                      color: widget.isAlert
                                        ? Colors.red.shade900
                                        : const Color(0xFF0D47A1),
                                    ),
                                  ),
                                  Text(
                                    vital.createdAt.toString().substring(0, 19),
                                    style: TextStyle(
                                      fontSize: 12,
                                      color: Colors.grey.shade600,
                                    ),
                                  ),
                                ],
                              ),
                              const SizedBox(height: 12),
                              Row(
                                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                children: [
                                  Column(
                                    crossAxisAlignment: CrossAxisAlignment.start,
                                    children: [
                                      Text(
                                        'Heart Rate',
                                        style: TextStyle(
                                          fontSize: 12,
                                          color: Colors.grey.shade600,
                                        ),
                                      ),
                                      Text(
                                        '${vital.heartrate} bpm',
                                        style: const TextStyle(
                                          fontSize: 16,
                                          fontWeight: FontWeight.bold,
                                        ),
                                      ),
                                    ],
                                  ),
                                  Column(
                                    crossAxisAlignment: CrossAxisAlignment.start,
                                    children: [
                                      Text(
                                        'Oxygen',
                                        style: TextStyle(
                                          fontSize: 12,
                                          color: Colors.grey.shade600,
                                        ),
                                      ),
                                      Text(
                                        '${vital.oxygen.toStringAsFixed(1)}%',
                                        style: const TextStyle(
                                          fontSize: 16,
                                          fontWeight: FontWeight.bold,
                                        ),
                                      ),
                                    ],
                                  ),
                                  Column(
                                    crossAxisAlignment: CrossAxisAlignment.start,
                                    children: [
                                      Text(
                                        'Confidence',
                                        style: TextStyle(
                                          fontSize: 12,
                                          color: Colors.grey.shade600,
                                        ),
                                      ),
                                      Text(
                                        '${(vital.confidence * 100).toStringAsFixed(0)}%',
                                        style: const TextStyle(
                                          fontSize: 16,
                                          fontWeight: FontWeight.bold,
                                        ),
                                      ),
                                    ],
                                  ),
                                ],
                              ),
                            ],
                          ),
                        );
                      },
                    );
                  },
                ),
              ],
            ),
          );
        },
      ),
    );
  }
}
