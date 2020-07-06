// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_DB_ERROR_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_DB_ERROR_HPP_

namespace altintegration {

namespace db {

/**
 * @class UnrecoverableError
 * @brief Application is not able to recover from the error occurred in the
 * storage.
 *
 * Example: database is corrupted and can not be opened.
 *
 * It is up to implementation to decide a full list of such error types.
 */
struct UnrecoverableError : public std::exception {};

/**
 * @class Error
 * @brief Application in normal condition can recover from this type of error.
 *
 * Example: out of disk space, IO error.
 *
 * It is up to implementation to decide a full list of such error types.
 */
struct Error : public std::exception {};

class DbError : public Error {
 public:
  /**
   * Constructor (C strings).
   * @param message C-style string error message.
   * The string contents are copied upon construction.
   * Hence, responsibility for deleting the char* lies
   * with the caller.
   */
  explicit DbError(const char* message) : msg_(message) {}

  /**
   * Constructor (C++ STL strings).
   * @param message The error message.
   */
  explicit DbError(const std::string& message) : msg_(message) {}

  /**
   * Destructor.
   * Virtual to allow for subclassing.
   */
  virtual ~DbError() throw() {}

  /**
   * Returns a pointer to the (constant) error description.
   * @return A pointer to a const char*. The underlying memory
   * is in posession of the Exception object. Callers must
   * not attempt to free the memory.
   */
  virtual const char* what() const throw() { return msg_.c_str(); }

 protected:
  /**
   * Error message.
   */
  std::string msg_;
};

struct StateCorruptedException : public std::domain_error {
  using base = std::domain_error;
  using base::base;  // to inherit constructors
};

struct BadIOException : public std::domain_error {
  using base = std::domain_error;
  using base::base;  // to inherit constructors
};

}  // namespace db

}  // namespace altintegration

#endif  // ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_DB_ERROR_HPP_
