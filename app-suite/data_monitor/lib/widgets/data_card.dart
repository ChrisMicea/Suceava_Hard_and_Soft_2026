import 'package:flutter/material.dart';

class DataCard extends StatelessWidget {
  final String title;
  final String icon;
  final String value;
  final String unit;
  final VoidCallback onTap;
  final bool isAlert;

  const DataCard({
    super.key,
    required this.title,
    required this.icon,
    required this.value,
    required this.unit,
    required this.onTap,
    this.isAlert = false,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.all(20),
        decoration: BoxDecoration(
          color: isAlert
            ? Colors.red.shade100
            : const Color(0xFFE3F2FD),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(
            color: isAlert
              ? Colors.red.shade300
              : const Color(0xFF64B5F6),
            width: 2,
          ),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(0.08),
              blurRadius: 8,
              offset: const Offset(0, 2),
            ),
          ],
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  title,
                  style: TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.w600,
                    color: isAlert
                      ? Colors.red.shade800
                      : const Color(0xFF1976D2),
                  ),
                ),
                Text(
                  icon,
                  style: const TextStyle(fontSize: 24),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Row(
              textBaseline: TextBaseline.alphabetic,
              children: [
                Text(
                  value,
                  style: TextStyle(
                    fontSize: 32,
                    fontWeight: FontWeight.bold,
                    color: isAlert
                      ? Colors.red.shade900
                      : const Color(0xFF0D47A1),
                  ),
                ),
                const SizedBox(width: 8),
                Text(
                  unit,
                  style: TextStyle(
                    fontSize: 14,
                    color: isAlert
                      ? Colors.red.shade700
                      : const Color(0xFF1976D2),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Text(
              'Tap to see details',
              style: TextStyle(
                fontSize: 12,
                color: isAlert
                  ? Colors.red.shade600
                  : Colors.grey.shade600,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
