BEGIN TRANSACTION;

CREATE TABLE zs_struct (
    identifier_id INTEGER PRIMARY KEY
                          REFERENCES zs_identifier (id) ON DELETE CASCADE,
    scope_id      INTEGER REFERENCES zs_object_scope (id),
    native        BOOLEAN,
    version       TEXT
);

COMMIT TRANSACTION;
