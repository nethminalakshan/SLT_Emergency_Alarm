"""
SLT Lift Master Alarm System — Logging Setup
=============================================
Configures centralized logging with both console and rotating file output.
All log entries include timestamps, severity levels, and module names.

Usage:
    from logger_setup import setup_logging
    logger = setup_logging(config)
"""

import os
import logging
from logging.handlers import RotatingFileHandler


def setup_logging(log_config: dict) -> logging.Logger:
    """
    Configure and return the application logger.

    Sets up two handlers:
      - Console handler (stdout) for systemd journal capture
      - Rotating file handler for persistent log storage

    Args:
        log_config: Dictionary with keys:
            - level (str): Logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
            - file (str): Path to the log file
            - max_bytes (int): Maximum log file size before rotation
            - backup_count (int): Number of rotated log files to keep

    Returns:
        logging.Logger: Configured logger instance
    """
    logger = logging.getLogger("slt_master_alarm")
    logger.setLevel(getattr(logging, log_config.get("level", "INFO").upper()))

    # Prevent duplicate handlers on re-initialization
    if logger.handlers:
        logger.handlers.clear()

    # Log format: timestamp — level — module — message
    formatter = logging.Formatter(
        fmt="%(asctime)s — %(levelname)-8s — %(name)s — %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S"
    )

    # --- Console Handler (captured by systemd journal) ---
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)

    # --- Rotating File Handler ---
    log_file = log_config.get("file", "/var/log/slt-master-alarm/alarm.log")
    log_dir = os.path.dirname(log_file)

    try:
        if log_dir and not os.path.exists(log_dir):
            os.makedirs(log_dir, exist_ok=True)

        file_handler = RotatingFileHandler(
            filename=log_file,
            maxBytes=log_config.get("max_bytes", 10485760),     # 10 MB default
            backupCount=log_config.get("backup_count", 5),
            encoding="utf-8"
        )
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
        logger.info(f"Log file initialized: {log_file}")

    except (PermissionError, OSError) as e:
        logger.warning(
            f"Cannot write to log file '{log_file}': {e}. "
            f"Logging to console only."
        )

    return logger
