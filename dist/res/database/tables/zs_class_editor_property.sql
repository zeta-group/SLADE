BEGIN TRANSACTION;

CREATE TABLE zs_class_editor_property (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE,
    name          TEXT,
    value         TEXT,
    UNIQUE (
        identifier_id,
        name
    )
);

COMMIT TRANSACTION;
