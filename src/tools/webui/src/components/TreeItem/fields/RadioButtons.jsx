/**
 * @file
 *
 * @brief radio buttons for tree items
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 */

import React, { Component } from "react";

import { Radio, RadioGroup } from "@material-ui/core";

import { fromElektraBool } from "../../../utils";

export default class RadioButtons extends Component {
  constructor(props) {
    super(props);
    this.state = { value: props.value || false };
  }

  componentWillReceiveProps(nextProps) {
    this.setState({ value: nextProps.value || false });
  }

  render() {
    const { id, value, meta, options, onChange } = this.props;
    const val = this.state.value === false ? value : this.state.value;

    return (
      <RadioGroup aria-label={id} name={id} value={val} onChange={(evt, value) => onChange(value)}
        onChange={(evt, value) => onChange(value)}
        style={{
          display: "inline-block",
          position: "relative",
          top: 7,
          marginTop: -11
        }}
      >
        {options.map(option => (
        <FormControlLabel
          value={option}
          control={<Radio />}
          label={option}
          style={{ display: "inline-block", width: "auto", paddingRight: 32 }}
          key={id + "-" + option}
          disabled={
                (meta && meta.hasOwnProperty("binary")) ||
                fromElektraBool(meta && meta["restrict/write"])} />
        ))}
      </RadioGroup>
    );
  }
}
