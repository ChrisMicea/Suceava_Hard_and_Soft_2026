class VitalSigns {
  final int id;
  final int heartrate;
  final double oxygen;
  final double confidence;
  final DateTime createdAt;

  VitalSigns({
    required this.id,
    required this.heartrate,
    required this.oxygen,
    required this.confidence,
    required this.createdAt,
  });

  factory VitalSigns.fromJson(Map<String, dynamic> json) {
    return VitalSigns(
      id: json['id'],
      heartrate: json['heartrate'],
      oxygen: (json['oxygen'] as num).toDouble(),
      confidence: (json['confidence'] as num).toDouble(),
      createdAt: DateTime.parse(json['createdAt']),
    );
  }
}

class Temperature {
  final int id;
  final double temperature;
  final DateTime createdAt;

  Temperature({
    required this.id,
    required this.temperature,
    required this.createdAt,
  });

  factory Temperature.fromJson(Map<String, dynamic> json) {
    return Temperature(
      id: json['id'],
      temperature: (json['temperature'] as num).toDouble(),
      createdAt: DateTime.parse(json['createdAt']),
    );
  }
}

class Motion {
  final int id;
  final double rotX;
  final double rotY;
  final double rotZ;
  final double accX;
  final double accY;
  final double accZ;
  final DateTime createdAt;

  Motion({
    required this.id,
    required this.rotX,
    required this.rotY,
    required this.rotZ,
    required this.accX,
    required this.accY,
    required this.accZ,
    required this.createdAt,
  });

  factory Motion.fromJson(Map<String, dynamic> json) {
    return Motion(
      id: json['id'],
      rotX: (json['rotX'] as num).toDouble(),
      rotY: (json['rotY'] as num).toDouble(),
      rotZ: (json['rotZ'] as num).toDouble(),
      accX: (json['accX'] as num).toDouble(),
      accY: (json['accY'] as num).toDouble(),
      accZ: (json['accZ'] as num).toDouble(),
      createdAt: DateTime.parse(json['createdAt']),
    );
  }
}

class PanicEvent {
  final int id;
  final String eventType;
  final String status;
  final DateTime createdAt;
  final DateTime? acknowledgedAt;

  PanicEvent({
    required this.id,
    required this.eventType,
    required this.status,
    required this.createdAt,
    this.acknowledgedAt,
  });

  factory PanicEvent.fromJson(Map<String, dynamic> json) {
    return PanicEvent(
      id: json['id'],
      eventType: json['eventType'],
      status: json['status'],
      createdAt: DateTime.parse(json['createdAt']),
      acknowledgedAt: json['acknowledgedAt'] != null
        ? DateTime.parse(json['acknowledgedAt'])
        : null,
    );
  }
}
