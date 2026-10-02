package org.example.backend.service;

import org.example.backend.model.PanicAlert;
import org.example.backend.repository.PanicAlertRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;

@Service
public class PanicAlertService {

    @Autowired
    private PanicAlertRepository panicAlertRepository;

    @Transactional
    public PanicAlert savePanicEvent(PanicAlert panicAlert) {
        return panicAlertRepository.save(panicAlert);
    }

    public Optional<PanicAlert> getLatestActivePanicEvent() {
        return panicAlertRepository.findFirstByStatusOrderByCreatedAtDesc("ACTIVE");
    }

    public List<PanicAlert> getActivePanicEvents() {
        return panicAlertRepository.findByStatusOrderByCreatedAtDesc("ACTIVE");
    }

    @Transactional
    public Optional<PanicAlert> acknowledgePanicEvent(Long id) {
        Optional<PanicAlert> panicEvent = panicAlertRepository.findById(id);
        if (panicEvent.isPresent()) {
            PanicAlert event = panicEvent.get();
            event.setStatus("ACKNOWLEDGED");
            event.setAcknowledgedAt(LocalDateTime.now());
            panicAlertRepository.save(event);
        }
        return panicEvent;
    }

    public List<PanicAlert> getAllPanicEvents() {
        return panicAlertRepository.findAll();
    }

    public Optional<PanicAlert> getPanicEventById(Long id) {
        return panicAlertRepository.findById(id);
    }
}
