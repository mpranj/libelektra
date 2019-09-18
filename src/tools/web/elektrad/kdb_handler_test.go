package main

import (
	"net/http"
	"testing"
)

func TestGetKdb(t *testing.T) {
	keyName := "user/elektrad/test/getKdb"
	keyNameChild := "user/elektrad/test/getKdb/child"

	setupKey(t, keyName, keyNameChild)

	w := testGet(t, "/kdb/"+keyName)

	code := w.Result().StatusCode
	Assertf(t, code == http.StatusOK, "wrong status code: %v", code)

	var response lookupResult

	parseBody(t, w, &response)

	Assert(t, response.Exists, "key not found")
	Assert(t, response.Path == keyName, "key path is wrong")
	CompareStrings(t, []string{keyName, keyNameChild}, response.Ls, "Children are not the same")
}

func TestPutKdb(t *testing.T) {
	keyName := "user/elektrad/test/putKdb"
	value := "test me"

	w := testPut(t, "/kdb/"+keyName, value)

	code := w.Result().StatusCode
	Assertf(t, code == http.StatusOK, "wrong status code: %v", code)

	key := getKey(t, keyName)
	retrievedValue := key.Value()
	Assert(t, key != nil, "key was not created")
	Assertf(t, retrievedValue == value, "wrong key value %s, expected %s", retrievedValue, value)
}
