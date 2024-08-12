// https://github.com/mongodb/mongo/blob/master/src/mongo/base/error_codes.yml

#if !defined(NOSQL_ERROR)
#error nosqlerror.hh cannot be included without NOSQL_ERROR being defined.
#endif

// The "location" errors are not documented, but appears to be created
// more or less on the spot in the MongoDB code and used for fringe cases.

#define NOSQL_LOCATION_ERROR(num)\
    NOSQL_ERROR(LOCATION ## num, num, "Location" #num)

// *INDENT-OFF*
NOSQL_ERROR(OK,                                    0, "OK")
NOSQL_ERROR(INTERNAL_ERROR,                        1, "InternalError")
NOSQL_ERROR(BAD_VALUE,                             2, "BadValue")
NOSQL_ERROR(NO_SUCH_KEY,                           4, "NoSuchKey")
NOSQL_ERROR(FAILED_TO_PARSE,                       9, "FailedToParse")
NOSQL_ERROR(USER_NOT_FOUND,                       11, "UserNotFound")
NOSQL_ERROR(UNSUPPORTED_FORMAT,                   12, "UnsupportedFormat")
NOSQL_ERROR(UNAUTHORIZED,                         13, "Unauthorized")
NOSQL_ERROR(TYPE_MISMATCH,                        14, "TypeMismatch")
NOSQL_ERROR(INVALID_LENGTH,                       16, "InvalidLength")
NOSQL_ERROR(PROTOCOL_ERROR,                       17, "ProtocolError")
NOSQL_ERROR(AUTHENTICATION_FAILED,                18, "AuthenticationFailed")
NOSQL_ERROR(NAMESPACE_NOT_FOUND,                  26, "NamespaceNotFound")
NOSQL_ERROR(ROLE_NOT_FOUND,                       31, "RoleNotFound")
NOSQL_ERROR(CONFLICTING_UPDATE_OPERATORS,         40, "ConflictingUpdateOperators")
NOSQL_ERROR(CURSOR_NOT_FOUND,                     43, "CursorNotFound")
NOSQL_ERROR(NAMESPACE_EXISTS,                     48, "NamespaceExists")
NOSQL_ERROR(DOLLAR_PREFIXED_FIELD_NAME,           52, "DollarPrefixedFieldName")
NOSQL_ERROR(EMPTY_FIELD_NAME,                     56, "EmptyFieldName")
NOSQL_ERROR(COMMAND_NOT_FOUND,                    59, "CommandNotFound")
NOSQL_ERROR(IMMUTABLE_FIELD,                      66, "ImmutableField")
NOSQL_ERROR(CANNOT_CREATE_INDEX,                  67, "CannotCreateIndex")
NOSQL_ERROR(INVALID_OPTIONS,                      72, "InvalidOptions")
NOSQL_ERROR(INVALID_NAMESPACE,                    73, "InvalidNamespace")
NOSQL_ERROR(NO_REPLICATION_ENABLED,               76, "NoReplicationEnabled")
NOSQL_ERROR(COMMAND_NOT_SUPPORTED,               115, "CommandNotSupported")
NOSQL_ERROR(COMMAND_FAILED,                      125, "CommandFailed")
NOSQL_ERROR(INVALID_PIPELINE_OPERATOR,           168, "InvalidPipelineOperator")
NOSQL_ERROR(CLIENT_METADATA_MISSING_FIELD,       183, "ClientMetadataMissingField")
NOSQL_ERROR(CLIENT_METADATA_CANNOT_BE_MUTATED,   186, "ClientMetadataCannotBeMutated")
NOSQL_ERROR(CONVERSION_FAILURE,                  241, "ConversionFailure")
NOSQL_ERROR(MECHANISM_UNAVAILABLE,               334, "MechanismUnavailable")
NOSQL_ERROR(DUPLICATE_KEY,                     11000, "DuplicateKey")

NOSQL_LOCATION_ERROR(10065)
NOSQL_LOCATION_ERROR(15947)
NOSQL_LOCATION_ERROR(15952)
NOSQL_LOCATION_ERROR(15955)
NOSQL_LOCATION_ERROR(15958)
NOSQL_LOCATION_ERROR(15973)
NOSQL_LOCATION_ERROR(15974)
NOSQL_LOCATION_ERROR(15975)
NOSQL_LOCATION_ERROR(15976)
NOSQL_LOCATION_ERROR(15959)
NOSQL_LOCATION_ERROR(15969)
NOSQL_LOCATION_ERROR(15981)
NOSQL_LOCATION_ERROR(16020)
NOSQL_LOCATION_ERROR(16610)
NOSQL_LOCATION_ERROR(16611)
NOSQL_LOCATION_ERROR(17312)
NOSQL_LOCATION_ERROR(17419)
NOSQL_LOCATION_ERROR(17420)
NOSQL_LOCATION_ERROR(28714)
NOSQL_LOCATION_ERROR(28745)
NOSQL_LOCATION_ERROR(28746)
NOSQL_LOCATION_ERROR(28747)
NOSQL_LOCATION_ERROR(28748)
NOSQL_LOCATION_ERROR(28749)
NOSQL_LOCATION_ERROR(28756)
NOSQL_LOCATION_ERROR(28757)
NOSQL_LOCATION_ERROR(28758)
NOSQL_LOCATION_ERROR(28759)
NOSQL_LOCATION_ERROR(28761)
NOSQL_LOCATION_ERROR(28762)
NOSQL_LOCATION_ERROR(28763)
NOSQL_LOCATION_ERROR(28764)
NOSQL_LOCATION_ERROR(28765)
NOSQL_LOCATION_ERROR(28808)
NOSQL_LOCATION_ERROR(28811)
NOSQL_LOCATION_ERROR(28812)
NOSQL_LOCATION_ERROR(28818)
NOSQL_LOCATION_ERROR(28822)
NOSQL_LOCATION_ERROR(31002)
NOSQL_LOCATION_ERROR(31032)
NOSQL_LOCATION_ERROR(31120)
NOSQL_LOCATION_ERROR(31160)
NOSQL_LOCATION_ERROR(31174)
NOSQL_LOCATION_ERROR(31175)
NOSQL_LOCATION_ERROR(31253)
NOSQL_LOCATION_ERROR(31254)
NOSQL_LOCATION_ERROR(31310)
NOSQL_LOCATION_ERROR(40156)
NOSQL_LOCATION_ERROR(40160)
NOSQL_LOCATION_ERROR(40234)
NOSQL_LOCATION_ERROR(40238)
NOSQL_LOCATION_ERROR(40272)
NOSQL_LOCATION_ERROR(40323)
NOSQL_LOCATION_ERROR(40324)
NOSQL_LOCATION_ERROR(40352)
NOSQL_LOCATION_ERROR(40353)
NOSQL_LOCATION_ERROR(40414)
NOSQL_LOCATION_ERROR(40602)
NOSQL_LOCATION_ERROR(51003)
NOSQL_LOCATION_ERROR(51270)
NOSQL_LOCATION_ERROR(51272)
NOSQL_LOCATION_ERROR(2107201)
NOSQL_LOCATION_ERROR(5107200)
NOSQL_LOCATION_ERROR(5107201)
NOSQL_LOCATION_ERROR(5447000)
NOSQL_LOCATION_ERROR(6047401)
// *INDENT-ON*
