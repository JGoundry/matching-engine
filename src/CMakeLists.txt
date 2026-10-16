add_library(matching-engine)
target_compile_features(matching-engine PUBLIC cxx_std_23)
set_target_properties(matching-engine PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON) # validate header files
target_sources(matching-engine
  PRIVATE
    MatchingEngine.cpp
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS .
    FILES
      Types.h
      Order.h
      OrderPool.h
      OrderBook.h
      InboundCommand.h
      OutboundResponse.h
      MatchingEngine.h
)
