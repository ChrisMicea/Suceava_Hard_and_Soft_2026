package org.example.backend.controller;

import org.example.backend.dto.TemperatureRequest;
import org.example.backend.model.Temperature;
import org.example.backend.service.TemperatureService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import java.util.List;
import java.util.Optional;

@RestController
@RequestMapping("/api/temperature")
public class TemperatureController {

    @Value("${api.auth.token}")
    private String validToken;

    @Autowired
    private TemperatureService temperatureService;

    @PostMapping
    public ResponseEntity<?> postTemperature(
            @RequestHeader(value = "Authorization", required = false) String authHeader,
            @RequestBody TemperatureRequest request) {

        if (!validateToken(authHeader)) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body("Invalid or missing token");
        }

        if (request.getTemperature() == null) {
            return ResponseEntity.badRequest()
                    .body("Missing required field: temperature");
        }

        Temperature temperature = new Temperature(request.getTemperature());

        Temperature saved = temperatureService.saveTemperature(temperature);
        return ResponseEntity.status(HttpStatus.CREATED).body(saved);
    }

    @GetMapping
    public ResponseEntity<List<Temperature>> getAllTemperature() {
        List<Temperature> temperatureList = temperatureService.getAllTemperature();
        return ResponseEntity.ok(temperatureList);
    }

    @GetMapping("/{id}")
    public ResponseEntity<?> getTemperatureById(@PathVariable Long id) {
        Optional<Temperature> temperature = temperatureService.getTemperatureById(id);
        if (temperature.isPresent()) {
            return ResponseEntity.ok(temperature.get());
        }
        return ResponseEntity.notFound().build();
    }

    @GetMapping("/latest")
    public ResponseEntity<?> getLatestTemperature() {
        Optional<Temperature> temperature = temperatureService.getLatestTemperature();
        if (temperature.isPresent()) {
            return ResponseEntity.ok(temperature.get());
        }
        return ResponseEntity.notFound().build();
    }

    private boolean validateToken(String authHeader) {
        if (authHeader == null || !authHeader.startsWith("Bearer ")) {
            return false;
        }
        String token = authHeader.substring(7);
        return validToken.equals(token);
    }
}
