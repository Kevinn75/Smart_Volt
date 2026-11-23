class SystemConfigModel {
  String user;
  int rooms;
  String r0;
  String r1;
  String r2;
  String ssid;
  String pass;
  double kwhPriceCop; // Tarifa en pesos colombianos por kWh (ej: 850 COP)

  SystemConfigModel({
    this.user = 'Mi Hogar',
    this.rooms = 3,
    this.r0 = 'Tablero Principal',
    this.r1 = 'Habitación 1',
    this.r2 = 'Cocina / Habitación 2',
    this.ssid = '',
    this.pass = '',
    this.kwhPriceCop = 850.0,
  });

  factory SystemConfigModel.fromJson(Map<String, dynamic> json) {
    return SystemConfigModel(
      user: json['user'] ?? 'Mi Hogar',
      rooms: json['rooms'] ?? 3,
      r0: json['r0'] ?? 'Tablero Principal',
      r1: json['r1'] ?? 'Habitación 1',
      r2: json['r2'] ?? 'Cocina / Habitación 2',
      ssid: json['ssid'] ?? '',
      pass: '',
      kwhPriceCop: 850.0,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'user': user,
      'rooms': rooms,
      'r0': r0,
      'r1': r1,
      'r2': r2,
      'ssid': ssid,
      'pass': pass,
    };
  }
}
