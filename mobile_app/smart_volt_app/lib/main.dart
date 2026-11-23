import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'providers/energy_provider.dart';
import 'screens/home_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const SmartVoltApp());
}

class SmartVoltApp extends StatelessWidget {
  const SmartVoltApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => EnergyProvider()),
      ],
      child: MaterialApp(
        title: 'Smart Volt Mobile',
        debugShowCheckedModeBanner: false,
        themeMode: ThemeMode.dark,
        darkTheme: ThemeData(
          useMaterial3: true,
          brightness: Brightness.dark,
          scaffoldBackgroundColor: const Color(0xFF0B1120),
          colorScheme: const ColorScheme.dark(
            primary: Color(0xFF0284C7),
            secondary: Color(0xFF38BDF8),
            surface: Color(0xFF1E293B),
            background: Color(0xFF0B1120),
          ),
          cardTheme: const CardTheme(
            color: Color(0xFF1E293B),
            elevation: 2,
          ),
          navigationBarTheme: NavigationBarThemeData(
            backgroundColor: const Color(0xFF0F172A),
            indicatorColor: const Color(0xFF0284C7).withOpacity(0.3),
            labelTextStyle: MaterialStateProperty.all(
              const TextStyle(fontSize: 12, fontWeight: FontWeight.w600),
            ),
          ),
        ),
        home: const HomeScreen(),
      ),
    );
  }
}
