BEGIN TRANSACTION;

CREATE TABLE zs_source (
    id              INTEGER PRIMARY KEY,
    archive_file_id INTEGER REFERENCES archive_file (id) ON DELETE CASCADE,
    entry_path      TEXT,
    UNIQUE (
        archive_file_id,
        entry_path
    )
);

COMMIT TRANSACTION;
