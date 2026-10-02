package org.example.backend.controller;

import org.example.backend.dto.MotionRequest;
import org.example.backend.model.Motion;
import org.example.backend.service.MotionService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import java.util.List;
import java.util.Optional;

@RestController
@RequestMapping("/api/motion")
public class MotionController {

    @Value("${api.auth.token}")
    private String validToken;

    @Autowired
    private MotionService motionService;

    @PostMapping
    public ResponseEntity<?> postMotion(
            @RequestHeader(value = "Authorization", required = false) String authHeader,
            @RequestBody MotionRequest request) {

        if (!validateToken(authHeader)) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED)
                    .body("Invalid or missing token");
        }

        if (request.getRotX() == null || request.getRotY() == null || request.getRotZ() == null ||
            request.getAccX() == null || request.getAccY() == null || request.getAccZ() == null) {
            return ResponseEntity.badRequest()
                    .body("Missing required fields: rotX, rotY, rotZ, accX, accY, accZ");
        }

        Motion motion = new Motion(
                request.getRotX(),
                request.getRotY(),
                request.getRotZ(),
                request.getAccX(),
                request.getAccY(),
                request.getAccZ()
        );

        Motion saved = motionService.saveMotion(motion);
        return ResponseEntity.status(HttpStatus.CREATED).body(saved);
    }

    @GetMapping
    public ResponseEntity<List<Motion>> getAllMotion() {
        List<Motion> motionList = motionService.getAllMotion();
        return ResponseEntity.ok(motionList);
    }

    @GetMapping("/{id}")
    public ResponseEntity<?> getMotionById(@PathVariable Long id) {
        Optional<Motion> motion = motionService.getMotionById(id);
        if (motion.isPresent()) {
            return ResponseEntity.ok(motion.get());
        }
        return ResponseEntity.notFound().build();
    }

    @GetMapping("/latest")
    public ResponseEntity<?> getLatestMotion() {
        Optional<Motion> motion = motionService.getLatestMotion();
        if (motion.isPresent()) {
            return ResponseEntity.ok(motion.get());
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
