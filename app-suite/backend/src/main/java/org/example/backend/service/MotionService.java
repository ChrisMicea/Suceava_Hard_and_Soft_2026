package org.example.backend.service;

import org.example.backend.model.Motion;
import org.example.backend.repository.MotionRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import java.util.List;
import java.util.Optional;

@Service
public class MotionService {

    @Autowired
    private MotionRepository motionRepository;

    @Transactional
    public Motion saveMotion(Motion motion) {
        return motionRepository.save(motion);
    }

    public List<Motion> getAllMotion() {
        return motionRepository.findAll();
    }

    public Optional<Motion> getMotionById(Long id) {
        return motionRepository.findById(id);
    }

    public Optional<Motion> getLatestMotion() {
        return motionRepository.findFirstByOrderByCreatedAtDesc();
    }
}
