
PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

CREATE TABLE archive_file (
    path          TEXT     NOT NULL
                           PRIMARY KEY,
    size          INTEGER,
    md5           TEXT,
    format_id     TEXT,
    last_opened   DATETIME,
    last_modified DATETIME
);

CREATE TABLE base_resource_path (
    path TEXT NOT NULL
              PRIMARY KEY
);

CREATE TABLE cvar (
    name  TEXT PRIMARY KEY
               NOT NULL,
    value BLOB
);

CREATE TABLE keybind (
    keybind_id TEXT    NOT NULL,
    alt        BOOLEAN,
    ctrl       BOOLEAN,
    shift      BOOLEAN,
    [key]      TEXT
);

CREATE TABLE nodebuilder_path (
    nodebuilder_id TEXT PRIMARY KEY
                        NOT NULL,
    path           TEXT
);

CREATE TABLE window_info (
    id        TEXT    PRIMARY KEY
                      NOT NULL,
    [left]    INTEGER,
    top       INTEGER,
    width     INTEGER,
    height    INTEGER,
    maximised BOOLEAN
);

COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
