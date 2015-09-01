libiri is a simple toolkit for parsing Internationalized Resource Identifiers (IRIs).

For many intents and purposes, you can think of libiri as a “URL parser which supports Unicode”.

Specifically:

  * URLs are a subset of URIs
  * URIs are restricted to a portion of the ASCII character set
  * IRIs are a superset of URIs that are not restricted to ASCII characters
  * If something is a valid URL or URI, it's also a valid IRI.