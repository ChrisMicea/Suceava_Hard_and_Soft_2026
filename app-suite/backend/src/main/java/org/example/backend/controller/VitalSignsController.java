package org.example.backend.controller;

import org.example.backend.dto.VitalSignsRequest;
import org.example.backend.model.VitalSigns;
import org.example.backend.service.VitalSignsService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import java.util.List;
import java.util.Optional;

@RestController
@RequestMapping("/api/vital-signs")
public class VitalSignsController {

    @Value("${api.auth.token}")
    private String validToken;

    @Autowired
    private VitalSignsService vitalSignsService;

    @PostMapping
    public ResponseEntity<?> postVitalSigns(
            @RequestHeader(value = "Authorization", required = false) String authHeader,
            @RequestBody VitalSignsRequest request) {

        if (!validateToken(authHeader)) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body("Invalid or missing token");
        }

        if (request.getHeartrate() == null || request.getOxygen() == null ||
            request.getConfidence() == null) {
            return ResponseEntity.badRequest()
                    .body("Missing required fields: heartrate, oxygen, confidence");
        }

        VitalSigns vitalSigns = new VitalSigns(
                request.getHeartrate(),
                request.getOxygen(),
                request.getConfidence()
        );

        VitalSigns saved = vitalSignsService.saveVitalSigns(vitalSigns);
        return ResponseEntity.status(HttpStatus.CREATED).body(saved);
    }

    @GetMapping
    public ResponseEntity<List<VitalSigns>> getAllVitalSigns() {
        List<VitalSigns> vitalSignsList = vitalSignsService.getAllVitalSigns();
        return ResponseEntity.ok(vitalSignsList);
    }

    @GetMapping("/{id}")
    public ResponseEntity<?> getVitalSignsById(@PathVariable Long id) {
        Optional<VitalSigns> vitalSigns = vitalSignsService.getVitalSignsById(id);
        if (vitalSigns.isPresent()) {
            return ResponseEntity.ok(vitalSigns.get());
        }
        return ResponseEntity.notFound().build();
    }

    @GetMapping("/latest")
    public ResponseEntity<?> getLatestVitalSigns() {
        Optional<VitalSigns> vitalSigns = vitalSignsService.getLatestVitalSigns();
        if (vitalSigns.isPresent()) {
            return ResponseEntity.ok(vitalSigns.get());
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
