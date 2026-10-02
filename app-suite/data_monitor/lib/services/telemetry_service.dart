import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:data_monitor/models/telemetry_models.dart';

class TelemetryService {
  static const String baseUrl = 'http://172.20.100.87:8080';
  static const String token = 'esp32_static_token_12345';

  final http.Client _client = http.Client();

  Future<List<VitalSigns>> getVitalSigns() async {
    try {
      final response = await _client.get(
        Uri.parse('$baseUrl/api/vital-signs'),
      );

      if (response.statusCode == 200) {
        final List<dynamic> json = jsonDecode(response.body);
        return json.map((item) => VitalSigns.fromJson(item as Map<String, dynamic>)).toList();
      }
      throw Exception('Failed to load vital signs: ${response.statusCode}');
    } catch (e) {
      throw Exception('Error fetching vital signs: $e');
    }
  }

  Future<VitalSigns?> getLatestVitalSigns() async {
    try {
      final response = await _client.get(
        Uri.parse('$baseUrl/api/vital-signs/latest'),
      );

      if (response.statusCode == 200) {
        return VitalSigns.fromJson(jsonDecode(response.body));
      } else if (response.statusCode == 404) {
        return null;
      }
      throw Exception('Failed to load latest vital signs: ${response.statusCode}');
    } catch (e) {
      throw Exception('Error fetching latest vital signs: $e');
    }
  }

  Future<List<Temperature>> getTemperature() async {
    try {
      final response = await _client.get(
        Uri.parse('$baseUrl/api/temperature'),
      );

      if (response.statusCode == 200) {
        final List<dynamic> json = jsonDecode(response.body);
        return json.map((item) => Temperature.fromJson(item as Map<String, dynamic>)).toList();
      }
      throw Exception('Failed to load temperature: ${response.statusCode}');
    } catch (e) {
      throw Exception('Error fetching temperature: $e');
    }
  }

  Future<Temperature?> getLatestTemperature() async {
    try {
      final response = await _client.get(
        Uri.parse('$baseUrl/api/temperature/latest'),
      );

      if (response.statusCode == 200) {
        return Temperature.fromJson(jsonDecode(response.body));
      } else if (response.statusCode == 404) {
        return null;
      }
      throw Exception('Failed to load latest temperature: ${response.statusCode}');
    } catch (e) {
      throw Exception('Error fetching latest temperature: $e');
    }
  }

  Future<List<Motion>> getMotion() async {
    try {
      final response = await _client.get(
        Uri.parse('$baseUrl/api/motion'),
      );

      if (response.statusCode == 200) {
        final List<dynamic> json = jsonDecode(response.body);
        return json.map((item) => Motion.fromJson(item as Map<String, dynamic>)).toList();
      }
      throw Exception('Failed to load motion: ${response.statusCode}');
    } catch (e) {
      throw Exception('Error fetching motion: $e');
    }
  }

  Future<Motion?> getLatestMotion() async {
    try {
      final response = await _client.get(
        Uri.parse('$baseUrl/api/motion/latest'),
      );

      if (response.statusCode == 200) {
        return Motion.fromJson(jsonDecode(response.body));
      } else if (response.statusCode == 404) {
        return null;
      }
      throw Exception('Failed to load latest motion: ${response.statusCode}');
    } catch (e) {
      throw Exception('Error fetching latest motion: $e');
    }
  }

  Future<Map<String, dynamic>?> getLatestPanicEvent() async {
    try {
      final response = await _client.get(
        Uri.parse('$baseUrl/api/panic-events/latest'),
      );

      if (response.statusCode == 200) {
        return jsonDecode(response.body);
      } else if (response.statusCode == 404) {
        return null;
      }
      throw Exception('Failed to load latest panic event: ${response.statusCode}');
    } catch (e) {
      throw Exception('Error fetching latest panic event: $e');
    }
  }

  Future<List<Map<String, dynamic>>> getAllPanicEvents() async {
    try {
      final response = await _client.get(
        Uri.parse('$baseUrl/api/panic-events'),
      );

      if (response.statusCode == 200) {
        final List<dynamic> json = jsonDecode(response.body);
        return json.cast<Map<String, dynamic>>();
      }
      throw Exception('Failed to load panic events: ${response.statusCode}');
    } catch (e) {
      throw Exception('Error fetching panic events: $e');
    }
  }

  Future<void> acknowledgeAlert(int alertId) async {
    try {
      final response = await _client.put(
        Uri.parse('$baseUrl/api/panic-events/$alertId/acknowledge'),
        headers: {
          'Authorization': 'Bearer $token',
          'Content-Type': 'application/json',
        },
      );

      if (response.statusCode != 200) {
        throw Exception('Failed to acknowledge alert: ${response.statusCode}');
      }
    } catch (e) {
      throw Exception('Error acknowledging alert: $e');
    }
  }

  Future<List<VitalSigns>> getTodaysVitalSigns() async {
    final today = DateTime.now();
    final todayStart = DateTime(today.year, today.month, today.day);
    final todayEnd = DateTime(today.year, today.month, today.day, 23, 59, 59);

    final allData = await getVitalSigns();
    return allData
        .where((vital) => vital.createdAt.isAfter(todayStart) && vital.createdAt.isBefore(todayEnd))
        .toList();
  }

  Future<List<Temperature>> getTodaysTemperature() async {
    final today = DateTime.now();
    final todayStart = DateTime(today.year, today.month, today.day);
    final todayEnd = DateTime(today.year, today.month, today.day, 23, 59, 59);

    final allData = await getTemperature();
    return allData
        .where((temp) => temp.createdAt.isAfter(todayStart) && temp.createdAt.isBefore(todayEnd))
        .toList();
  }

  Future<List<Motion>> getTodaysMotion() async {
    final today = DateTime.now();
    final todayStart = DateTime(today.year, today.month, today.day);
    final todayEnd = DateTime(today.year, today.month, today.day, 23, 59, 59);

    final allData = await getMotion();
    return allData
        .where((motion) => motion.createdAt.isAfter(todayStart) && motion.createdAt.isBefore(todayEnd))
        .toList();
  }
}
