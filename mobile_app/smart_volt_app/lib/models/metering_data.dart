class ChannelData {
  final int id;
  final String name;
  final double current;       // Amperes
  final double activePower;   // Watts
  final double apparentPower; // VA
  final double reactivePower; // VAR
  final double powerFactor;   // [-1, 1]
  final double energyKwh;     // kWh

  ChannelData({
    required this.id,
    required this.name,
    required this.current,
    required this.activePower,
    required this.apparentPower,
    required this.reactivePower,
    required this.powerFactor,
    required this.energyKwh,
  });

  factory ChannelData.fromJson(Map<String, dynamic> json, int id) {
    return ChannelData(
      id: id,
      name: json['name'] ?? 'Canal $id',
      current: (json['i'] as num?)?.toDouble() ?? 0.0,
      activePower: (json['p'] as num?)?.toDouble() ?? 0.0,
      apparentPower: (json['s'] as num?)?.toDouble() ?? 0.0,
      reactivePower: (json['q'] as num?)?.toDouble() ?? 0.0,
      powerFactor: (json['fp'] as num?)?.toDouble() ?? 0.0,
      energyKwh: (json['e'] as num?)?.toDouble() ?? 0.0,
    );
  }
}

class MeteringData {
  final double voltage;       // Vrms
  final String user;          // Nombre usuario
  final String status;        // Estado de red
  final String ip;            // IP ESP32
  final int roomsCount;       // Habitaciones activas
  final double totalPower;    // Suma P (W)
  final double totalCurrent;  // Suma I (A)
  final double totalEnergy;   // Suma kWh
  final List<ChannelData> channels;
  final DateTime timestamp;

  MeteringData({
    required this.voltage,
    required this.user,
    required this.status,
    required this.ip,
    required this.roomsCount,
    required this.totalPower,
    required this.totalCurrent,
    required this.totalEnergy,
    required this.channels,
    required this.timestamp,
  });

  factory MeteringData.fromJson(Map<String, dynamic> json) {
    List<ChannelData> chs = [];
    if (json['channels'] != null) {
      var list = json['channels'] as List;
      for (int i = 0; i < list.length; i++) {
        chs.add(ChannelData.fromJson(list[i], i + 1));
      }
    }

    return MeteringData(
      voltage: (json['v'] as num?)?.toDouble() ?? 120.0,
      user: json['user'] ?? 'Mi Hogar',
      status: json['status'] ?? 'Conectado',
      ip: json['ip'] ?? '192.168.4.1',
      roomsCount: json['rooms'] ?? chs.length,
      totalPower: (json['tot_p'] as num?)?.toDouble() ?? 0.0,
      totalCurrent: (json['tot_i'] as num?)?.toDouble() ?? 0.0,
      totalEnergy: (json['tot_e'] as num?)?.toDouble() ?? 0.0,
      channels: chs,
      timestamp: DateTime.now(),
    );
  }

  // Generador de datos simulados para pruebas cuando el ESP32 no esté conectado
  factory MeteringData.mock() {
    return MeteringData(
      voltage: 121.2,
      user: 'Casa de Muestra',
      status: 'Modo Demostración (Offline)',
      ip: '192.168.4.1',
      roomsCount: 3,
      totalPower: 642.0,
      totalCurrent: 5.35,
      totalEnergy: 0.245,
      timestamp: DateTime.now(),
      channels: [
        ChannelData(
          id: 1,
          name: 'Tablero General (Principal)',
          current: 4.82,
          activePower: 580.0,
          apparentPower: 585.0,
          reactivePower: 76.0,
          powerFactor: 0.99,
          energyKwh: 0.210,
        ),
        ChannelData(
          id: 2,
          name: 'Habitación 1 (Iluminación/TV)',
          current: 0.53,
          activePower: 62.0,
          apparentPower: 64.0,
          reactivePower: 16.0,
          powerFactor: 0.97,
          energyKwh: 0.035,
        ),
        ChannelData(
          id: 3,
          name: 'Cocina / Nevera',
          current: 0.00,
          activePower: 0.0,
          apparentPower: 0.0,
          reactivePower: 0.0,
          powerFactor: 0.00,
          energyKwh: 0.000,
        ),
      ],
    );
  }
}
