/*jslint browser: true, devel: true, node: true, ass: true, nomen: true, unparam: true, indent: 4 */

/**
 * @license
 * Copyright (c) 2015 The ExpandJS authors. All rights reserved.
 * This code may only be used under the BSD style license found at https://expandjs.github.io/LICENSE.txt
 * The complete set of authors may be found at https://expandjs.github.io/AUTHORS.txt
 * The complete set of contributors may be found at https://expandjs.github.io/CONTRIBUTORS.txt
 */
(function () {
    "use strict";

    var lodash         = require('lodash'),
        assertArgument = require('../assert/assertArgument'),
        isString       = require('../tester/isString'),
        isVoid         = require('../tester/isVoid');

    /**
     * Checks if `string` ends with the given target string.
     *
     * ```js
     * XP.endsWith('hello_world', 'world');
     * // => true
     *
     * XP.endsWith('hello_world', 'world', ' ');
     * // => false
     * ```
     *
     * @function endsWith
     * @param {string} [string = ""] The string to search.
     * @param {string} [target = ""] The string to search for.
     * @param {string} [spacer = ""] The spacer between `string` and `target`.
     * @returns {boolean} Returns `true` if `string` ends with `target`, else `false`.
     */
    module.exports = function endsWith(string, target, spacer) {
        assertArgument(isVoid(string) || isString(string), 1, 'string');
        assertArgument(isVoid(target) || isString(target), 2, 'string');
        assertArgument(isVoid(spacer) || isString(spacer), 3, 'string');
        return lodash.endsWith(string, (spacer || '') + (target || ''));
    };

}());