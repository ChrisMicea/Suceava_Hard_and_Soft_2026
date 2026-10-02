package org.example.backend.controller;

import org.example.backend.dto.PanicAlertRequest;
import org.example.backend.model.PanicAlert;
import org.example.backend.service.PanicAlertService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import java.util.List;
import java.util.Optional;

@RestController
@RequestMapping("/api/panic-events")
public class PanicAlertController {

    @Value("${api.auth.token}")
    private String validToken;

    @Autowired
    private PanicAlertService panicAlertService;

    @PostMapping
    public ResponseEntity<?> postPanicEvent(
            @RequestHeader(value = "Authorization", required = false) String authHeader,
            @RequestBody PanicAlertRequest request) {

        if (!validateToken(authHeader)) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body("Invalid or missing token");
        }

        if (request.getEventType() == null) {
            return ResponseEntity.badRequest()
                    .body("Missing required field: eventType");
        }

        PanicAlert panicEvent = new PanicAlert(request.getEventType());
        PanicAlert saved = panicAlertService.savePanicEvent(panicEvent);
        return ResponseEntity.status(HttpStatus.CREATED).body(saved);
    }

    @GetMapping("/latest")
    public ResponseEntity<?> getLatestPanicEvent() {
        Optional<PanicAlert> panicEvent = panicAlertService.getLatestActivePanicEvent();
        if (panicEvent.isPresent()) {
            return ResponseEntity.ok(panicEvent.get());
        }
        return ResponseEntity.notFound().build();
    }

    @GetMapping
    public ResponseEntity<List<PanicAlert>> getAllPanicEvents() {
        List<PanicAlert> panicEvents = panicAlertService.getAllPanicEvents();
        return ResponseEntity.ok(panicEvents);
    }

    @GetMapping("/{id}")
    public ResponseEntity<?> getPanicEventById(@PathVariable Long id) {
        Optional<PanicAlert> panicEvent = panicAlertService.getPanicEventById(id);
        if (panicEvent.isPresent()) {
            return ResponseEntity.ok(panicEvent.get());
        }
        return ResponseEntity.notFound().build();
    }

    @PutMapping("/{id}/acknowledge")
    public ResponseEntity<?> acknowledgePanicEvent(@PathVariable Long id) {
        Optional<PanicAlert> panicEvent = panicAlertService.acknowledgePanicEvent(id);
        if (panicEvent.isPresent()) {
            return ResponseEntity.ok(panicEvent.get());
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
