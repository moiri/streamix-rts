Creator "igraph version 0.7.1 StreamixC"
Version 1
graph
[
  directed 1
  node
  [
    id 0
    label "A"
    func "a"
  ]
  node
  [
    id 1
    label "B"
    func "b"
  ]
  edge
  [
    source 1
    target 0
    label "syn"
  ]
  edge
  [
    source 0
    target 1
    label "syn_ack"
  ]
  edge
  [
    source 1
    target 0
    label "ack"
  ]
]
